(function () {
  'use strict';

  if (!window.WebGLRenderingContext) {
    console.log('[ThreeJS] WebGL not supported - skipping 3D scene');
    return;
  }

  let reduceMotion = false;
  if (typeof window.matchMedia === 'function') {
    reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  }

  const isMobile = window.innerWidth < 768;
  const particleCount = isMobile ? 60 : 160;

  const canvas = document.getElementById('three-canvas');
  if (!canvas) return;

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
  camera.position.z = 5;

  const renderer = new THREE.WebGLRenderer({
    canvas: canvas,
    alpha: true,
    antialias: !isMobile,
    powerPreference: 'high-performance',
  });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));

  let particles = null;

  function initParticles() {
    var geometry = new THREE.BufferGeometry();
    var positions = new Float32Array(particleCount * 3);
    var velocities = new Float32Array(particleCount * 3);

    for (var i = 0; i < particleCount; i++) {
      positions[i * 3] = (Math.random() - 0.5) * 12;
      positions[i * 3 + 1] = (Math.random() - 0.5) * 14 - 2;
      positions[i * 3 + 2] = (Math.random() - 0.5) * 8 - 2;
      velocities[i * 3] = (Math.random() - 0.5) * 0.03;
      velocities[i * 3 + 1] = Math.random() * 0.07 + 0.04;
      velocities[i * 3 + 2] = (Math.random() - 0.5) * 0.03;
    }

    geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));

    var material = new THREE.PointsMaterial({
      color: 0xff4500,
      size: 0.05,
      transparent: true,
      opacity: reduceMotion ? 0.1 : 0.25,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });

    particles = new THREE.Points(geometry, material);
    particles.userData = { velocities: velocities };
    scene.add(particles);
  }

  let mouseX = 0;
  let mouseY = 0;
  let targetX = 0;
  let targetY = 0;

  if (!reduceMotion && !isMobile) {
    document.addEventListener('mousemove', function (e) {
      mouseX = (e.clientX / window.innerWidth) * 2 - 1;
      mouseY = -(e.clientY / window.innerHeight) * 2 + 1;
    });
  }

  let time = 0;
  let isVisible = true;

  document.addEventListener('visibilitychange', function () {
    isVisible = !document.hidden;
  });

  function animate() {
    requestAnimationFrame(animate);

    if (!isVisible) return;

    time += 0.016;

    if (particles) {
      var positions = particles.geometry.attributes.position.array;
      var velocities = particles.userData.velocities;
      var spd = reduceMotion ? 0.05 : 0.8;

      for (var i = 0; i < particleCount; i++) {
        var i3 = i * 3;
        var windX = Math.sin(time * 4 + i * 0.7) * 0.005 + Math.sin(time * 1.3 + i * 2.1) * 0.003;
        var windZ = Math.cos(time * 4 + i * 0.7) * 0.005 + Math.cos(time * 1.3 + i * 2.1) * 0.003;

        positions[i3] += (velocities[i3] + windX) * spd;
        positions[i3 + 1] += velocities[i3 + 1] * spd;
        positions[i3 + 2] += (velocities[i3 + 2] + windZ) * spd;

        if (!reduceMotion && Math.random() < 0.003) {
          positions[i3] += (Math.random() - 0.5) * 0.25;
          positions[i3 + 1] += Math.random() * 0.2;
          positions[i3 + 2] += (Math.random() - 0.5) * 0.25;
        }

        if (positions[i3 + 1] > 6) {
          positions[i3] = (Math.random() - 0.5) * 12;
          positions[i3 + 1] = -6;
          positions[i3 + 2] = (Math.random() - 0.5) * 8 - 2;
        }
      }

      particles.geometry.attributes.position.needsUpdate = true;

      if (!reduceMotion) {
        particles.rotation.y = Math.sin(time * 0.3) * 0.06;
      }
    }

    if (!reduceMotion && !isMobile) {
      targetX += (mouseX - targetX) * 0.02;
      targetY += (mouseY - targetY) * 0.02;
      camera.position.x = targetX * 0.3;
      camera.position.y = targetY * 0.3;
      camera.lookAt(scene.position);
    }

    renderer.render(scene, camera);
  }

  function onResize() {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  }

  window.addEventListener('resize', onResize);

  initParticles();
  animate();

  console.log('[ThreeJS] Scene initialized -', particleCount, 'particles');
})();
