// COLD Banking Suite - Interactive JavaScript

document.addEventListener('DOMContentLoaded', function() {
    // Initialize all components
    initializeTabSwitching();
    initializeScrollEffects();
    initializeWalletConnection();
    initializeFormHandlers();
    initializeAnimations();
    initializeStatsCounter();
});

// Tab switching functionality
function initializeTabSwitching() {
    const tabButtons = document.querySelectorAll('.tab-btn');
    const tabContents = document.querySelectorAll('.tab-content');

    tabButtons.forEach(button => {
        button.addEventListener('click', () => {
            const targetTab = button.getAttribute('data-tab');
            
            // Remove active class from all tabs and contents
            tabButtons.forEach(btn => btn.classList.remove('active'));
            tabContents.forEach(content => content.classList.remove('active'));
            
            // Add active class to clicked tab and corresponding content
            button.classList.add('active');
            document.getElementById(`${targetTab}-tab`).classList.add('active');
            
            // Add subtle animation
            const activeContent = document.getElementById(`${targetTab}-tab`);
            activeContent.style.opacity = '0';
            activeContent.style.transform = 'translateY(20px)';
            
            setTimeout(() => {
                activeContent.style.transition = 'all 0.3s ease';
                activeContent.style.opacity = '1';
                activeContent.style.transform = 'translateY(0)';
            }, 50);
        });
    });
}

// Scroll effects for navigation
function initializeScrollEffects() {
    const header = document.querySelector('.header');
    let lastScrollY = window.scrollY;

    window.addEventListener('scroll', () => {
        const currentScrollY = window.scrollY;
        
        // Add/remove header background based on scroll
        if (currentScrollY > 50) {
            header.style.background = 'rgba(0, 0, 0, 0.95)';
            header.style.backdropFilter = 'blur(20px)';
        } else {
            header.style.background = 'rgba(0, 0, 0, 0.9)';
            header.style.backdropFilter = 'blur(10px)';
        }
        
        lastScrollY = currentScrollY;
    });

    // Smooth scrolling for navigation links
    document.querySelectorAll('a[href^="#"]').forEach(anchor => {
        anchor.addEventListener('click', function (e) {
            e.preventDefault();
            const target = document.querySelector(this.getAttribute('href'));
            if (target) {
                target.scrollIntoView({
                    behavior: 'smooth',
                    block: 'start'
                });
            }
        });
    });
}

// Wallet connection functionality
function initializeWalletConnection() {
    const connectButton = document.querySelector('.connect-wallet-btn');
    let isConnected = false;

    connectButton.addEventListener('click', async () => {
        if (!isConnected) {
            // Simulate wallet connection
            connectButton.textContent = 'Connecting...';
            connectButton.disabled = true;
            
            // Add loading animation
            connectButton.style.background = 'linear-gradient(45deg, #666, #999)';
            
            setTimeout(() => {
                isConnected = true;
                connectButton.textContent = 'Wallet Connected';
                connectButton.style.background = 'linear-gradient(45deg, #00ff00, #00ffff)';
                connectButton.disabled = false;
                
                // Show success animation
                showNotification('Wallet connected successfully!', 'success');
                updateWalletInfo();
            }, 2000);
        } else {
            // Disconnect wallet
            isConnected = false;
            connectButton.textContent = 'Connect Wallet';
            connectButton.style.background = 'linear-gradient(45deg, #00ffff, #ff00ff)';
            showNotification('Wallet disconnected', 'info');
            resetWalletInfo();
        }
    });
}

// Form handlers
function initializeFormHandlers() {
    // Deposit form
    const depositBtn = document.querySelector('.deposit-btn');
    const depositAmount = document.getElementById('deposit-amount');
    const recipientAddress = document.getElementById('recipient-address');

    if (depositBtn) {
        depositBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            
            const amount = depositAmount.value;
            const recipient = recipientAddress.value;
            
            if (!amount || amount <= 0) {
                showNotification('Please enter a valid deposit amount', 'error');
                return;
            }
            
            // Simulate proof generation and deposit
            depositBtn.textContent = 'Generating Proof...';
            depositBtn.disabled = true;
            
            setTimeout(() => {
                depositBtn.textContent = 'Submitting to Blockchain...';
                
                setTimeout(() => {
                    depositBtn.textContent = 'Generate Proof & Deposit';
                    depositBtn.disabled = false;
                    showNotification(`Successfully deposited ${amount} XFG!`, 'success');
                    
                    // Reset form
                    depositAmount.value = '';
                    recipientAddress.value = '';
                    
                    // Update stats
                    updateStats();
                }, 3000);
            }, 2000);
        });
    }

    // Withdraw form
    const withdrawBtn = document.querySelector('.withdraw-btn');
    const proofFile = document.getElementById('proof-file');

    if (withdrawBtn) {
        withdrawBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            
            if (!proofFile.files.length) {
                showNotification('Please upload a proof file', 'error');
                return;
            }
            
            // Simulate proof verification
            withdrawBtn.textContent = 'Verifying Proof...';
            withdrawBtn.disabled = true;
            
            setTimeout(() => {
                withdrawBtn.textContent = 'Claiming Rewards...';
                
                setTimeout(() => {
                    withdrawBtn.textContent = 'Verify Proof & Claim';
                    withdrawBtn.disabled = false;
                    showNotification('Successfully claimed 100 HEAT + 1 O!', 'success');
                    
                    // Reset form
                    proofFile.value = '';
                    updateWalletBalances();
                }, 2000);
            }, 1500);
        });
    }

    // File upload handler
    if (proofFile) {
        proofFile.addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (file) {
                const uploadText = document.querySelector('.upload-text span');
                uploadText.textContent = `Selected: ${file.name}`;
                showNotification('Proof file uploaded successfully', 'success');
            }
        });
    }

    // Voting handlers
    document.querySelectorAll('.vote-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const voteType = btn.classList.contains('yes') ? 'Yes' : 'No';
            
            btn.textContent = 'Voting...';
            btn.disabled = true;
            
            setTimeout(() => {
                btn.textContent = `Vote ${voteType}`;
                btn.disabled = false;
                showNotification(`Vote "${voteType}" submitted successfully!`, 'success');
            }, 1500);
        });
    });
}

// Enhanced animations
function initializeAnimations() {
    // Intersection Observer for scroll animations
    const observerOptions = {
        threshold: 0.1,
        rootMargin: '0px 0px -50px 0px'
    };

    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.style.opacity = '1';
                entry.target.style.transform = 'translateY(0)';
            }
        });
    }, observerOptions);

    // Observe all cards and sections
    document.querySelectorAll('.protocol-card, .token-card, .proposal-card').forEach(el => {
        el.style.opacity = '0';
        el.style.transform = 'translateY(30px)';
        el.style.transition = 'all 0.6s ease';
        observer.observe(el);
    });

    // Enhanced COLD letter animations
    const coldLetters = document.querySelectorAll('.cold-letter');
    coldLetters.forEach((letter, index) => {
        letter.addEventListener('mouseenter', () => {
            letter.style.transform = 'scale(1.1) rotateY(15deg)';
            letter.style.filter = 'brightness(1.5) drop-shadow(0 0 50px rgba(255, 0, 255, 1))';
        });
        
        letter.addEventListener('mouseleave', () => {
            letter.style.transform = 'scale(1) rotateY(0deg)';
            letter.style.filter = '';
        });
    });

    // Particle effect for buttons
    document.querySelectorAll('.primary-btn, .secondary-btn').forEach(btn => {
        btn.addEventListener('click', createParticleEffect);
    });
}

// Stats counter animation
function initializeStatsCounter() {
    const statValues = document.querySelectorAll('.stat-value');
    
    const animateCounter = (element, target, duration = 2000) => {
        const start = 0;
        const increment = target / (duration / 16);
        let current = start;
        
        const timer = setInterval(() => {
            current += increment;
            if (current >= target) {
                current = target;
                clearInterval(timer);
            }
            
            if (element.textContent.includes('$')) {
                element.textContent = `$${Math.floor(current).toLocaleString()}`;
            } else if (element.textContent.includes('∞')) {
                element.textContent = '∞';
            } else {
                element.textContent = Math.floor(current).toLocaleString();
            }
        }, 16);
    };

    // Trigger counter animation when stats come into view
    const statsObserver = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                const statValue = entry.target;
                const text = statValue.textContent;
                
                if (text.includes('$')) {
                    animateCounter(statValue, 1250000); // $1.25M TVL
                } else if (!text.includes('∞')) {
                    animateCounter(statValue, 847); // 847 deposits
                }
                
                statsObserver.unobserve(statValue);
            }
        });
    });

    statValues.forEach(stat => {
        if (!stat.textContent.includes('∞')) {
            statsObserver.observe(stat);
        }
    });
}

// Utility functions
function showNotification(message, type = 'info') {
    // Remove existing notifications
    const existingNotifications = document.querySelectorAll('.notification');
    existingNotifications.forEach(notif => notif.remove());

    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.innerHTML = `
        <div class="notification-content">
            <span class="notification-icon">${getNotificationIcon(type)}</span>
            <span class="notification-message">${message}</span>
        </div>
    `;

    // Add styles
    notification.style.cssText = `
        position: fixed;
        top: 100px;
        right: 20px;
        z-index: 10000;
        padding: 1rem 1.5rem;
        border-radius: 10px;
        backdrop-filter: blur(10px);
        border: 1px solid ${getNotificationColor(type)};
        background: rgba(0, 0, 0, 0.9);
        color: white;
        transform: translateX(100%);
        transition: all 0.3s ease;
        max-width: 400px;
        box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
    `;

    document.body.appendChild(notification);

    // Animate in
    setTimeout(() => {
        notification.style.transform = 'translateX(0)';
    }, 100);

    // Auto remove
    setTimeout(() => {
        notification.style.transform = 'translateX(100%)';
        setTimeout(() => notification.remove(), 300);
    }, 4000);
}

function getNotificationIcon(type) {
    const icons = {
        success: '✅',
        error: '❌',
        warning: '⚠️',
        info: 'ℹ️'
    };
    return icons[type] || icons.info;
}

function getNotificationColor(type) {
    const colors = {
        success: 'rgba(0, 255, 0, 0.5)',
        error: 'rgba(255, 0, 0, 0.5)',
        warning: 'rgba(255, 255, 0, 0.5)',
        info: 'rgba(0, 255, 255, 0.5)'
    };
    return colors[type] || colors.info;
}

function createParticleEffect(e) {
    const button = e.target;
    const rect = button.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    for (let i = 0; i < 6; i++) {
        const particle = document.createElement('div');
        particle.style.cssText = `
            position: absolute;
            left: ${x}px;
            top: ${y}px;
            width: 4px;
            height: 4px;
            background: #00ffff;
            border-radius: 50%;
            pointer-events: none;
            z-index: 1000;
        `;

        button.style.position = 'relative';
        button.appendChild(particle);

        const angle = (i / 6) * Math.PI * 2;
        const velocity = 50 + Math.random() * 50;
        const vx = Math.cos(angle) * velocity;
        const vy = Math.sin(angle) * velocity;

        let posX = x;
        let posY = y;
        let opacity = 1;

        const animate = () => {
            posX += vx * 0.02;
            posY += vy * 0.02;
            opacity -= 0.02;

            particle.style.left = posX + 'px';
            particle.style.top = posY + 'px';
            particle.style.opacity = opacity;

            if (opacity > 0) {
                requestAnimationFrame(animate);
            } else {
                particle.remove();
            }
        };

        animate();
    }
}

function updateWalletInfo() {
    // Update governance stats
    const govStats = document.querySelectorAll('.gov-stat .stat-value');
    if (govStats.length >= 2) {
        govStats[0].textContent = '2.5'; // O Balance
        govStats[1].textContent = '3.1%'; // Voting Power
    }
}

function resetWalletInfo() {
    const govStats = document.querySelectorAll('.gov-stat .stat-value');
    if (govStats.length >= 2) {
        govStats[0].textContent = '0.0';
        govStats[1].textContent = '0%';
    }
}

function updateWalletBalances() {
    // Simulate updating wallet balances after claiming
    showNotification('Wallet balances updated', 'info');
}

function updateStats() {
    // Simulate updating protocol stats
    const statValues = document.querySelectorAll('.stat-value');
    if (statValues.length >= 2) {
        // Update TVL
        const currentTVL = parseInt(statValues[0].textContent.replace(/[$,]/g, '')) || 0;
        statValues[0].textContent = `$${(currentTVL + 1000).toLocaleString()}`;
        
        // Update deposits
        const currentDeposits = parseInt(statValues[1].textContent.replace(/,/g, '')) || 0;
        statValues[1].textContent = (currentDeposits + 1).toLocaleString();
    }
}

// Add some dynamic background effects
function addDynamicEffects() {
    // Create floating particles
    const createFloatingParticle = () => {
        const particle = document.createElement('div');
        particle.style.cssText = `
            position: fixed;
            width: 2px;
            height: 2px;
            background: rgba(0, 255, 255, 0.6);
            border-radius: 50%;
            pointer-events: none;
            z-index: 2;
            left: ${Math.random() * window.innerWidth}px;
            top: ${window.innerHeight + 10}px;
        `;

        document.body.appendChild(particle);

        const duration = 10000 + Math.random() * 10000;
        const drift = (Math.random() - 0.5) * 100;

        particle.animate([
            { transform: `translateY(0px) translateX(0px)`, opacity: 0 },
            { transform: `translateY(-${window.innerHeight + 100}px) translateX(${drift}px)`, opacity: 1 }
        ], {
            duration: duration,
            easing: 'linear'
        }).onfinish = () => particle.remove();
    };

    // Create particles periodically
    setInterval(createFloatingParticle, 2000);
}

// Initialize dynamic effects
setTimeout(addDynamicEffects, 1000);