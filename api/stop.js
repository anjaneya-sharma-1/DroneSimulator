module.exports = (req, res) => {
  res.setHeader('Content-Type', 'application/json');
  res.statusCode = 501;
  res.json({
    error: 'backend_unavailable',
    message: 'Stopping simulation is not supported in this Vercel deployment. The native C backend cannot run in Vercel serverless functions. Please self-host the server or point the UI to a separate host that can run the C core.'
  });
};
