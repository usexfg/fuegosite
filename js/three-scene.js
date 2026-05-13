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
  const particleCount = isMobile ? 60 : 180;

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

  function initHeroParticles() {
    const geometry = new THREE.BufferGeometry();
    const positions = new Float32Array(particleCount * 3);
    const velocities = new Float32Array(particleCount * 3);

    for (let i = 0; i < particleCount; i++) {
      positions[i * 3] = (Math.random() - 0.5) * 12;
      positions[i * 3 + 1] = (Math.random() - 0.5) * 12 - 2;
      positions[i * 3 + 2] = (Math.random() - 0.5) * 8 - 2;
      velocities[i * 3] = (Math.random() - 0.5) * 0.015;
      velocities[i * 3 + 1] = Math.random() * 0.04 + 0.02;
      velocities[i * 3 + 2] = (Math.random() - 0.5) * 0.015;
    }

    geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));

    const material = new THREE.PointsMaterial({
      color: 0xff4500,
      size: 0.06,
      transparent: true,
      opacity: reduceMotion ? 0.15 : 0.3,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });

    particles = new THREE.Points(geometry, material);
    particles.userData = { velocities };
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
  let frameCount = 0;
  const skipFrames = reduceMotion ? 3 : 1;
  let isVisible = true;

  document.addEventListener('visibilitychange', function () {
    isVisible = !document.hidden;
  });

  function animate() {
    requestAnimationFrame(animate);

    if (!isVisible) return;

    frameCount++;
    if (frameCount % skipFrames !== 0 && !reduceMotion) {
      renderer.render(scene, camera);
      return;
    }

    time += 0.01;

    if (particles) {
      var positions = particles.geometry.attributes.position.array;
      var velocities = particles.userData.velocities;
      var speedMul = reduceMotion ? 0.05 : 0.6;

      for (var i = 0; i < particleCount; i++) {
        var i3 = i * 3;
        var windX = Math.sin(time * 2 + i) * 0.003;
        var windZ = Math.cos(time * 2 + i) * 0.003;

        positions[i3] += (velocities[i3] + windX) * speedMul;
        positions[i3 + 1] += velocities[i3 + 1] * speedMul;
        positions[i3 + 2] += (velocities[i3 + 2] + windZ) * speedMul;

        if (!reduceMotion && Math.random() < 0.002) {
          positions[i3] += (Math.random() - 0.5) * 0.2;
          positions[i3 + 1] += Math.random() * 0.15;
          positions[i3 + 2] += (Math.random() - 0.5) * 0.2;
        }

        if (positions[i3 + 1] > 6) {
          positions[i3] = (Math.random() - 0.5) * 12;
          positions[i3 + 1] = -6;
          positions[i3 + 2] = (Math.random() - 0.5) * 8 - 2;
        }
      }

      particles.geometry.attributes.position.needsUpdate = true;

      if (!reduceMotion && isMobile === false) {
        particles.rotation.y = Math.sin(time * 0.2) * 0.08;
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
    var newCount = window.innerWidth < 768 ? 60 : 180;

    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  }

  window.addEventListener('resize', onResize);

  initHeroParticles();
  animate();

  console.log('[ThreeJS] Scene initialized -', particleCount, 'particles');
})();
