// Placeholder serverless function for Vercel deployment
// NOTE: The original project uses a C core (pthreads, semaphores) which cannot run in Vercel serverless functions.
// This endpoint intentionally returns 501 Not Implemented and points users to self-hosting options.

module.exports = (req, res) => {
  res.setHeader('Content-Type', 'application/json');
  res.statusCode = 501;
  res.json({
    error: 'simulation_backend_unavailable',
    message: 'The native C simulation core cannot run in Vercel serverless functions. Please self-host the server component or run the C core separately and configure this UI to point to that host.',
    guidance: {
      self_hosting: 'Build the C core (cd src/c_core && make) and run src/web/server/index.js on a Linux/WSL or other server. Then configure the UI to send simulation requests to that host.',
      alternative: 'If you want a fully serverless JS implementation, consider porting the simulation core to Node.js or WebAssembly (this repository currently does not include that).'
    }
  });
};
