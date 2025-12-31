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

  window.SimUI = {
    setupCanvas,
    bindRange,
  };
})();
