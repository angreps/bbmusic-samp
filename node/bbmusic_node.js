/**
 * ==============================================================================
 *                     BBMusic SA-MP Node.js Sync Module
 *             Official samp-node / node-samp companion for BBMusic
 *                   Website: https://bbmusic.biz.id
 * ==============================================================================
 * 
 * Penggunaan di server GTA SA-MP yang menggunakan plugin samp-node:
 * 1. Letakkan file ini di folder server SA-MP Anda:
 *    Contoh: server/packages/bbmusic/index.js (atau require('./bbmusic_node') di script utama)
 * 2. Modul ini secara otomatis mengambil daftar lagu terbaru dari bbmusic.biz.id secara realtime
 * 3. Menembakkan callback ke Pawn saat ada lagu baru yang dikonversi di web
 * ==============================================================================
 */

const https = require('https');

const BBMUSIC_API_URL = 'https://bbmusic.biz.id/api/samp/tracks?format=json';
const SYNC_INTERVAL_MS = 60000; // Cek update setiap 60 detik

let cachedTracks = [];

function fetchTracks(onComplete) {
  https.get(BBMUSIC_API_URL, (res) => {
    let data = '';
    res.on('data', chunk => { data += chunk; });
    res.on('end', () => {
      try {
        const json = JSON.parse(data);
        if (json.success && Array.isArray(json.tracks)) {
          const prevCount = cachedTracks.length;
          const wasEmpty = prevCount === 0;
          cachedTracks = json.tracks;
          
          // Jika ada lagu baru yang baru saja di-convert di web
          if (!wasEmpty && json.tracks.length > prevCount) {
            const newTrack = json.tracks[0];
            console.log(`[BBMusic Node] Lagu baru terdeteksi dari website: "${newTrack.title}" oleh ${newTrack.artist}`);
            
            // Panggil callback ke Pawn jika berjalan di lingkungan samp-node
            if (typeof samp !== 'undefined' && samp.callLocal) {
              samp.callLocal('OnBBMusicWebTrackAdded', 'sss', newTrack.id, newTrack.title, newTrack.artist);
            }
          }

          if (typeof onComplete === 'function') {
            onComplete(true, cachedTracks);
          }
        }
      } catch (e) {
        if (typeof onComplete === 'function') onComplete(false, e.message);
      }
    });
  }).on('error', (err) => {
    console.error('[BBMusic Node] Gagal sync ke bbmusic.biz.id:', err.message);
    if (typeof onComplete === 'function') onComplete(false, err.message);
  });
}

// Inisialisasi awal saat server start & jalankan background loop
fetchTracks((ok, list) => {
  if (ok) {
    console.log(`[BBMusic Node] Berhasil sync awal: ${list.length} lagu siap digunakan di server SA-MP.`);
  }
});

setInterval(fetchTracks, SYNC_INTERVAL_MS);

module.exports = {
  getTracks: () => cachedTracks,
  syncNow: fetchTracks,
  formatStreamUrl: (trackId) => `https://bbmusic.biz.id/stream/${trackId}.mp3`,
};
