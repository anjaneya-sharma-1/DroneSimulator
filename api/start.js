module.exports = (req, res) => {
  res.setHeader('Content-Type', 'application/json');
  res.statusCode = 501;
  res.json({
    error: 'backend_unavailable',
    message: 'Starting simulation is not supported in this Vercel deployment. The native C backend cannot run in Vercel serverless functions. Please self-host the server or point the UI to a separate host that can run the C core.',
    guidance: {
      self_hosting: 'Build and run the C core on a separate server (cd src/c_core && make) and run src/web/server/index.js. Then configure the UI to POST to that host instead of /api/start.',
      alternative: 'Port the simulation core to Node.js or WebAssembly for serverless compatibility.'
    }
  });
};
