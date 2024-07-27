/// @file
/// @brief weForth - service worker for sleep handler
///
/// Note: currently only Safari support WorkerNavigator.serviceWorker
///       so, navigator.serviceWorker registration need to happen only in
///       the main module (or HTML)
///
addEventListener('install',  () => self.skipWaiting())
addEventListener('activate', () => self.clients.claim())
addEventListener('fetch', e => {
    // we only handle requests to a special /SLEEP url:
    const url = new URL(e.request.url)
    if (url.pathname !== '/SLEEP') return

    // wait ?t=X milliseconds, then return a 304:
    e.respondWith(
        new Promise(resolve => {
            const t = new URLSearchParams(url.search).get('t')
            const response = new Response(null, {status:304})
            setTimeout(resolve, t, response)
        }))
})
