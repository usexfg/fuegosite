// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

interface IFuegoCheckpointVerifier {
    struct Checkpoint {
        bytes32 checkpointHash;
        bytes32 merkleRoot;
        uint32  heightEnd;
    }
    function latest() external view returns (Checkpoint memory);
}

interface IHEATToken {
    function mint(address to, uint256 amount) external;
}

contract HEATClaimer {
    IFuegoCheckpointVerifier public immutable checkpointVerifier;
    IHEATToken public immutable heatToken;

    // HEAT is pegged 1:1 to XFG atomic units (satoshi-equivalent)
    // Adjust this multiplier if HEAT has a different decimal precision
    uint256 public constant HEAT_DECIMALS = 18;
    uint256 public constant XFG_DECIMALS  = 8;
    uint256 public constant HEAT_PER_XFG_ATOM = 10 ** (HEAT_DECIMALS - XFG_DECIMALS); // 1e10

    mapping(bytes32 => bool) public usedNullifiers;

    event HEATClaimed(
        bytes32 indexed commitment,
        bytes32 indexed nullifier,
        address indexed recipient,
        uint256 heatAmount
    );

    error AlreadyClaimed();
    error InvalidPreimageLength();
    error InvalidMerkleProof();

    constructor(address _checkpointVerifier, address _heatToken) {
        checkpointVerifier = IFuegoCheckpointVerifier(_checkpointVerifier);
        heatToken = IHEATToken(_heatToken);
    }

    /// Claim HEAT for a burned commitment.
    /// @param preimage  56-byte preimage: secret[32] || amount_le64[8] || network_id_le32[4] || chain_id_le32[4] || version_le32[4] || term_le32[4]
    /// @param merkleProof  Ordered sibling hashes from leaf to root
    /// @param leafIndex    Position of this commitment in the CommitmentIndex
    /// @param recipient    Address to receive HEAT tokens
    function claimHEAT(
        bytes  calldata preimage,
        bytes32[] calldata merkleProof,
        uint256 leafIndex,
        address recipient
    ) external {
        if (preimage.length != 56) revert InvalidPreimageLength();

        bytes32 commitment = keccak256(preimage);

        // Nullifier: keccak256(secret[0:32] || "nullifier" || amount_le64[32:40])
        bytes32 nullifier = keccak256(
            abi.encodePacked(bytes32(preimage[:32]), "nullifier", bytes8(preimage[32:40]))
        );
        if (usedNullifiers[nullifier]) revert AlreadyClaimed();

        bytes32 merkleRoot = checkpointVerifier.latest().merkleRoot;
        if (!_verifyMerkleProof(commitment, merkleProof, leafIndex, merkleRoot)) {
            revert InvalidMerkleProof();
        }

        usedNullifiers[nullifier] = true;

        // Decode amount: bytes 32-40 are LE uint64
        uint64 xfgAtoms = _decodeLE64(preimage[32:40]);
        uint256 heatAmount = uint256(xfgAtoms) * HEAT_PER_XFG_ATOM;

        heatToken.mint(recipient, heatAmount);

        emit HEATClaimed(commitment, nullifier, recipient, heatAmount);
    }

    /// Standard binary merkle proof verification using keccak256.
    /// Leaf ordering: even index = left child, odd index = right child.
    function _verifyMerkleProof(
        bytes32 leaf,
        bytes32[] calldata proof,
        uint256 index,
        bytes32 root
    ) internal pure returns (bool) {
        bytes32 current = leaf;
        for (uint256 i = 0; i < proof.length; i++) {
            if (index % 2 == 0) {
                current = keccak256(abi.encodePacked(current, proof[i]));
            } else {
                current = keccak256(abi.encodePacked(proof[i], current));
            }
            index /= 2;
        }
        return current == root;
    }

    /// Decode a little-endian uint64 from 8 bytes.
    function _decodeLE64(bytes calldata b) internal pure returns (uint64 v) {
        for (uint256 i = 0; i < 8; i++) {
            v |= uint64(uint8(b[i])) << (8 * i);
        }
    }
}
