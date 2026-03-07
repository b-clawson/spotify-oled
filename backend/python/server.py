#!/usr/bin/env python3
"""
Spotify Album Art Display - Backend Server (Python FastAPI)

This server:
1. Authenticates with Spotify using OAuth 2.0
2. Polls the "Currently Playing" endpoint
3. Caches the latest track information
4. Exposes a simple REST API for the ESP32
"""

import os
import time
import json
from typing import Optional
from fastapi import FastAPI, HTTPException
import spotipy
from spotipy.oauth2 import SpotifyOAuth
from dotenv import load_dotenv
import uvicorn

# Load environment variables
load_dotenv()

# =============================================================================
# Configuration
# =============================================================================

PORT = int(os.getenv("PORT", "8000"))
CLIENT_ID = os.getenv("SPOTIFY_CLIENT_ID")
CLIENT_SECRET = os.getenv("SPOTIFY_CLIENT_SECRET")
REDIRECT_URI = os.getenv("SPOTIFY_REDIRECT_URI", "http://localhost:8888/callback")
REFRESH_TOKEN = os.getenv("SPOTIFY_REFRESH_TOKEN")  # Optional: for Railway deployment

# Validate configuration
if not CLIENT_ID or not CLIENT_SECRET:
    print("[ERROR] Missing Spotify credentials!")
    print("Set SPOTIFY_CLIENT_ID and SPOTIFY_CLIENT_SECRET in .env file")
    exit(1)

# Write cache from refresh token if provided (for Railway/production)
# This allows Railway to use a pre-authenticated refresh token
if REFRESH_TOKEN:
    try:
        print("[Spotify] SPOTIFY_REFRESH_TOKEN found, creating cache with refresh token")
        # Create cache with refresh token and minimal token info
        # Spotipy needs this structure to properly refresh tokens
        cache_data = {
            "refresh_token": REFRESH_TOKEN,
            "token_type": "Bearer",
            "scope": "user-read-currently-playing user-read-playback-state"
        }
        with open(".spotify_cache", "w") as f:
            json.dump(cache_data, f)
        print("[Spotify] Cache file created successfully with refresh token")

    except Exception as e:
        print(f"[Warning] Failed to write cache from refresh token: {e}")
        import traceback
        traceback.print_exc()
else:
    print("[Spotify] No SPOTIFY_REFRESH_TOKEN env var, checking for .spotify_cache file")
    if os.path.exists(".spotify_cache"):
        print("[Spotify] .spotify_cache file exists")
    else:
        print("[Spotify] WARNING: No cache file or env var found!")

# =============================================================================
# Spotify Client Setup
# =============================================================================

auth_manager = SpotifyOAuth(
    client_id=CLIENT_ID,
    client_secret=CLIENT_SECRET,
    redirect_uri=REDIRECT_URI,
    scope="user-read-currently-playing user-read-playback-state",
    cache_path=".spotify_cache"
)

sp = spotipy.Spotify(auth_manager=auth_manager)

# Don't test auth on startup - let it happen on first request
# This prevents Railway from hanging during container startup
print("[Spotify] Auth manager configured - will authenticate on first API request")

# =============================================================================
# Cache
# =============================================================================

class SimpleCache:
    def __init__(self, ttl=30):
        self.ttl = ttl
        self.cache = {}
        self.timestamps = {}

    def get(self, key):
        if key in self.cache:
            if time.time() - self.timestamps[key] < self.ttl:
                return self.cache[key]
            else:
                del self.cache[key]
                del self.timestamps[key]
        return None

    def set(self, key, value):
        self.cache[key] = value
        self.timestamps[key] = time.time()

cache = SimpleCache(ttl=30)

# =============================================================================
# FastAPI App
# =============================================================================

app = FastAPI(
    title="Spotify Album Art Display Backend",
    description="Backend API for ESP32 Spotify display",
    version="1.0.0"
)

# =============================================================================
# Helper Functions
# =============================================================================

def get_current_track() -> Optional[dict]:
    """Fetch currently playing track from Spotify."""
    try:
        print("[Spotify] Fetching current track...")
        current = sp.current_user_playing_track()
        print(f"[Spotify] Response: {current is not None}")

        if not current or not current.get("is_playing"):
            print("[Spotify] Nothing playing or not active")
            return None

        item = current.get("item")
        if not item:
            print("[Spotify] No item in response")
            return None

        # Extract track information
        track_data = {
            "id": item["id"],
            "playing": current["is_playing"],
            "artist": ", ".join(artist["name"] for artist in item["artists"]),
            "track": item["name"],
            "album": item["album"]["name"],
            "image": item["album"]["images"][0]["url"] if item["album"]["images"] else "",
            "progress_ms": current.get("progress_ms", 0),
            "duration_ms": item.get("duration_ms", 0),
        }

        print(f"[Spotify] Track: {track_data['artist']} - {track_data['track']}")
        return track_data

    except spotipy.exceptions.SpotifyException as e:
        print(f"[Spotify] API error: {e}")
        print(f"[Spotify] Error details: {e.http_status}, {e.msg}")
        return None
    except Exception as e:
        print(f"[Error] Unexpected error: {type(e).__name__}: {e}")
        import traceback
        traceback.print_exc()
        return None

# =============================================================================
# API Endpoints
# =============================================================================

@app.get("/")
async def root():
    """Root endpoint with API information."""
    return {
        "service": "Spotify Album Art Display Backend",
        "version": "1.0.0",
        "endpoints": {
            "/now-playing": "Get currently playing track",
            "/health": "Health check",
        }
    }

@app.get("/now-playing")
async def now_playing():
    """
    Get currently playing track information.

    Returns:
        JSON with track info if something is playing, otherwise playing=false
    """
    try:
        # Check cache first
        track_data = cache.get("current_track")

        if not track_data:
            # Fetch from Spotify
            track_data = get_current_track()

            if track_data:
                cache.set("current_track", track_data)

        if not track_data or not track_data.get("playing"):
            return {"playing": False}

        return {
            "playing": True,
            "id": track_data["id"],
            "artist": track_data["artist"],
            "track": track_data["track"],
            "album": track_data["album"],
            "image": track_data["image"],
            "progress_ms": track_data["progress_ms"],
            "duration_ms": track_data["duration_ms"],
        }

    except Exception as e:
        print(f"[API] Error: {e}")
        raise HTTPException(status_code=500, detail="Internal server error")

@app.get("/health")
async def health():
    """Health check endpoint."""
    return {
        "status": "ok",
        "timestamp": time.time(),
    }

# =============================================================================
# Background Task (Optional)
# =============================================================================

# You can add a background task to keep the cache warm
# For now, the cache is updated on-demand when /now-playing is called

# =============================================================================
# Main
# =============================================================================

if __name__ == "__main__":
    print("\n========================================")
    print("Spotify Display Backend (Python)")
    print("========================================")
    print(f"Server starting on http://localhost:{PORT}")
    print("\nEndpoints:")
    print("  GET /now-playing")
    print("  GET /health")
    print("\nPress Ctrl+C to stop")
    print("========================================\n")

    # Skip authentication test on startup to avoid hanging
    # Auth will be tested on first API request
    print("[Spotify] Server ready - authentication will be tested on first request")

    uvicorn.run(app, host="0.0.0.0", port=PORT)
