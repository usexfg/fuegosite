/**
 * Fuego Website 3D Scene
 * Subtle ember particles for hero section
 */

(function() {
  'use strict';

  // Check for WebGL support
  if (!window.WebGLRenderingContext) {
    console.log('[ThreeJS] WebGL not supported - skipping 3D scene');
    return;
  }

  // Check for reduced motion preference
  const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  
  // Mobile detection
  const isMobile = window.innerWidth < 768;
  const particleCount = isMobile ? 100 : 300;

  // Scene setup
  const canvas = document.getElementById('three-canvas');
  if (!canvas) return;

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
  camera.position.z = 5;

  const renderer = new THREE.WebGLRenderer({ canvas: canvas, alpha: true, antialias: !isMobile });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));

  // Hero particles - ember effect
  let particles = null;
  
  function initHeroParticles() {
    const geometry = new THREE.BufferGeometry();
    const positions = new Float32Array(particleCount * 3);
    const velocities = new Float32Array(particleCount * 3);
    const sizes = new Float32Array(particleCount);
    
    for (let i = 0; i < particleCount; i++) {
      positions[i * 3] = (Math.random() - 0.5) * 12;
      positions[i * 3 + 1] = (Math.random() - 0.5) * 12 - 2;
      positions[i * 3 + 2] = (Math.random() - 0.5) * 8 - 2;
      
      velocities[i * 3] = (Math.random() - 0.5) * 0.01;
      velocities[i * 3 + 1] = Math.random() * 0.03 + 0.01;
      velocities[i * 3 + 2] = (Math.random() - 0.5) * 0.01;
      
      sizes[i] = Math.random() * 0.1 + 0.01;
    }
    
    geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
    geometry.setAttribute('size', new THREE.BufferAttribute(sizes, 1));
    
    // Custom shader material for glow effect
    const material = new THREE.PointsMaterial({
      color: 0xFF4500,
      size: 0.05,
      transparent: true,
      opacity: reduceMotion ? 0.3 : 0.5,
      blending: THREE.AdditiveBlending,
      depthWrite: false
    });
    
    particles = new THREE.Points(geometry, material);
    particles.userData = { velocities, sizes };
    scene.add(particles);
  }

  // Mouse parallax
  let mouseX = 0;
  let mouseY = 0;
  let targetX = 0;
  let targetY = 0;
  
  if (!reduceMotion && !isMobile) {
    document.addEventListener('mousemove', (e) => {
      mouseX = (e.clientX / window.innerWidth) * 2 - 1;
      mouseY = -(e.clientY / window.innerHeight) * 2 + 1;
    });
  }

  // Animation loop
  let time = 0;
  
  function animate() {
    requestAnimationFrame(animate);
    
    time += 0.01;
    
    if (particles && !reduceMotion) {
      const positions = particles.geometry.attributes.position.array;
      const velocities = particles.userData.velocities;
      
      for (let i = 0; i < particleCount; i++) {
        // Wind effect: add subtle horizontal oscillation
        positions[i * 3] += velocities[i * 3] + Math.sin(time + i) * 0.002;
        positions[i * 3 + 1] += velocities[i * 3 + 1];
        positions[i * 3 + 2] += velocities[i * 3 + 2] + Math.cos(time + i) * 0.002;
        
        // "Pop" effect: occasional sudden upward boost
        if (Math.random() < 0.005) {
          positions[i * 3 + 1] += Math.random() * 0.1;
        }
        
        // Reset if out of view
        if (positions[i * 3 + 1] > 6) {
          positions[i * 3] = (Math.random() - 0.5) * 12;
          positions[i * 3 + 1] = -6;
          positions[i * 3 + 2] = (Math.random() - 0.5) * 8 - 2;
        }
      }
      
      particles.geometry.attributes.position.needsUpdate = true;
      
      // Subtle rotation
      particles.rotation.y = Math.sin(time * 0.1) * 0.05;
    }
    
    // Smooth parallax
    if (!reduceMotion && !isMobile) {
      targetX += (mouseX - targetX) * 0.02;
      targetY += (mouseY - targetY) * 0.02;
      camera.position.x = targetX * 0.3;
      camera.position.y = targetY * 0.3;
      camera.lookAt(scene.position);
    }
    
    renderer.render(scene, camera);
  }

  // Handle resize
  function onResize() {
    const newParticleCount = window.innerWidth < 768 ? 100 : 300;
    
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    
    // Rebuild particles if count changed significantly
    if (Math.abs(newParticleCount - particleCount) > 50) {
      if (particles) {
        scene.remove(particles);
        particles = null;
      }
      initHeroParticles();
    }
  }
  
  window.addEventListener('resize', onResize);

  // Initialize
  initHeroParticles();
  animate();

  console.log('[ThreeJS] Fuego 3D scene initialized -', particleCount, 'particles');
})();
