(() => {
  function setupCanvas(id) {
    const canvas = document.getElementById(id);
    if (!canvas) {
      throw new Error(`Canvas not found: ${id}`);
    }
    const ctx = canvas.getContext('2d');

    function resize() {
      const rect = canvas.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      canvas.width = Math.floor(rect.width * dpr);
      canvas.height = Math.floor(rect.height * dpr);
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }
    window.addEventListener('resize', resize);
    resize();

    return { canvas, ctx, resize };
  }

  function bindRange(rangeEl, outEl, fmt = (v) => v) {
    const upd = () => {
      outEl.textContent = fmt(rangeEl.value);
    };
    rangeEl.addEventListener('input', upd);
    upd();
    return upd;
  }

  function createDebugHud(getModel) {
    const hud = document.createElement('div');
    hud.style.position = 'fixed';
    hud.style.right = '12px';
    hud.style.bottom = '12px';
    hud.style.background = 'rgba(10,12,15,0.85)';
    hud.style.color = '#e8eef7';
    hud.style.border = '1px solid rgba(255,255,255,0.15)';
    hud.style.padding = '8px 10px';
    hud.style.borderRadius = '10px';
    hud.style.font = '12px/1.4 system-ui, sans-serif';
    hud.style.maxWidth = '320px';
    hud.style.display = 'none';
    hud.style.zIndex = '9999';
    hud.textContent = 'debug';
    document.body.appendChild(hud);

    function renderHud() {
      const model = getModel();
      hud.textContent = JSON.stringify(model, null, 2);
    }

    window.addEventListener('keydown', (e) => {
      if (e.key === 'd' || e.key === 'D') {
        hud.style.display = hud.style.display === 'none' ? 'block' : 'none';
        if (hud.style.display === 'block') {
          renderHud();
        }
      }
    });

    return renderHud;
  }

  window.SimUI = {
    setupCanvas,
    bindRange,
    createDebugHud,
  };
})();
