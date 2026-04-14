#!/usr/bin/env python3
"""
EzYTDownloader fetch backend — yt-dlp wrapper with JSON-line IPC.

Protocol:
  Every line printed to stdout is a complete JSON object with a "type" field:
    {"type": "metadata", ...}
    {"type": "progress", "percent": 45.2, "speed": "1.5MiB/s", "eta": "00:30"}
    {"type": "complete", "path": "/tmp/..."}
    {"type": "error",    "message": "..."}

Usage:
  python3 fetch.py --info <URL>
  python3 fetch.py --download <URL> --format <FMT> --output <PATH> [--merge-audio]
"""

import sys
import json
import argparse
import os
import glob

_real_stdout = sys.stdout


def emit(obj: dict) -> None:
    """Print a JSON object as a single line to the real stdout and flush."""
    try:
        line = json.dumps(obj, ensure_ascii=False)
        _real_stdout.write(line + "\n")
        _real_stdout.flush()
    except BrokenPipeError:
        sys.exit(1)


def emit_error(msg: str) -> None:
    emit({"type": "error", "message": str(msg)})


# ── Fetch metadata ──────────────────────────────────────────────────────────

def fetch_info(url: str) -> None:
    try:
        import yt_dlp
    except ImportError:
        emit_error("yt-dlp is not installed. Please install it first.")
        sys.exit(1)

    ydl_opts = {
        "quiet": True,
        "no_warnings": True,
        "skip_download": True,
        "noprogress": True,
    }

    try:
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            info = ydl.extract_info(url, download=False)
    except yt_dlp.utils.DownloadError as e:
        emit_error(f"Failed to fetch video info: {e}")
        sys.exit(1)
    except Exception as e:
        emit_error(f"Unexpected error fetching info: {e}")
        sys.exit(1)

    if info is None:
        emit_error("No video information returned.")
        sys.exit(1)

    title     = info.get("title", "Unknown")
    duration  = info.get("duration", 0) or 0
    thumbnail = info.get("thumbnail", "")
    formats   = info.get("formats", [])
    streams   = []

    for fmt in formats:
        fmt_id  = fmt.get("format_id", "")
        ext     = fmt.get("ext", "")
        vcodec  = fmt.get("vcodec", "none")
        acodec  = fmt.get("acodec", "none")
        height  = fmt.get("height")
        fps     = fmt.get("fps")
        tbr     = fmt.get("tbr")
        abr     = fmt.get("abr")
        filesize = fmt.get("filesize") or fmt.get("filesize_approx") or 0

        is_audio_only = (vcodec == "none" or vcodec is None) and acodec not in ("none", None)
        is_video      = vcodec not in ("none", None)

        if not is_audio_only and not is_video:
            continue

        if is_audio_only:
            resolution = "audio only"
            codec   = acodec.split(".")[0] if acodec else "unknown"
            bitrate = int(abr) if abr else (int(tbr) if tbr else 0)
        else:
            resolution = f"{height}p" if height else fmt.get("format_note", "?")
            codec   = vcodec.split(".")[0] if vcodec else "unknown"
            bitrate = int(tbr) if tbr else 0

        streams.append({
            "formatId":    fmt_id,
            "resolution":  resolution,
            "ext":         ext,
            "filesize":    int(filesize),
            "bitrate":     bitrate,
            "codec":       codec,
            "isAudioOnly": is_audio_only,
            "fps":         int(fps) if fps else 0,
        })

    def sort_key(s):
        if s["isAudioOnly"]:
            return (1, -s["bitrate"])
        try:
            h = int(s["resolution"].replace("p", ""))
        except (ValueError, AttributeError):
            h = 0
        return (0, -h)

    streams.sort(key=sort_key)

    emit({
        "type":      "metadata",
        "title":     title,
        "duration":  duration,
        "thumbnail": thumbnail,
        "streams":   streams,
    })


# ── Download ────────────────────────────────────────────────────────────────

def download_stream(url: str, format_id: str, output_path: str,
                    merge_audio: bool = False) -> None:
    try:
        import yt_dlp
    except ImportError:
        emit_error("yt-dlp is not installed.")
        sys.exit(1)

    # ── Format string ────────────────────────────────────────────────────
    if merge_audio:
        dl_format = f"{format_id}+bestaudio/{format_id}"
    else:
        dl_format = format_id

    # ── Progress hook — ONLY emits progress, NEVER complete ──────────────
    def progress_hook(d: dict) -> None:
        status = d.get("status", "")
        if status == "downloading":
            pct_raw = d.get("_percent_str", "0%").strip().replace("%", "")
            try:
                percent = float(pct_raw)
            except ValueError:
                percent = 0.0
            speed = d.get("_speed_str", "N/A").strip()
            eta   = d.get("_eta_str", "N/A").strip()
            emit({
                "type":    "progress",
                "percent": round(percent, 1),
                "speed":   speed,
                "eta":     eta,
            })
        # NOTE: We intentionally do NOT emit "complete" here.
        # We emit it exactly once after ydl.download() returns.

    # ── Suppress yt-dlp stdout ───────────────────────────────────────────
    devnull = open(os.devnull, "w")
    sys.stdout = devnull

    # ── yt-dlp options ───────────────────────────────────────────────────
    # Use the output_path as a LITERAL path (no %(ext)s template).
    # For merge, set merge_output_format to match the file extension.
    _, ext = os.path.splitext(output_path)
    ext = ext.lstrip(".")  # "mkv", "m4a", etc.

    ydl_opts = {
        "format":      dl_format,
        "outtmpl":     output_path,
        "quiet":       True,
        "no_warnings": True,
        "noprogress":  True,
        "progress_hooks": [progress_hook],
    }

    if merge_audio:
        # Tell yt-dlp to merge into the same format as our extension
        ydl_opts["merge_output_format"] = ext if ext else "mkv"

    try:
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            ydl.download([url])
    except yt_dlp.utils.DownloadError as e:
        sys.stdout = _real_stdout
        emit_error(f"Download failed: {e}")
        sys.exit(1)
    except Exception as e:
        sys.stdout = _real_stdout
        emit_error(f"Unexpected error during download: {e}")
        sys.exit(1)
    finally:
        sys.stdout = _real_stdout
        devnull.close()

    # ── Find the actual output file and emit exactly ONE complete ────────
    if os.path.exists(output_path):
        emit({"type": "complete", "path": output_path})
    else:
        # yt-dlp may have adjusted the extension; search for it
        base, _ = os.path.splitext(output_path)
        candidates = [f for f in glob.glob(base + ".*")
                      if not f.endswith(".part") and not ".f" in os.path.basename(f)]
        if candidates:
            actual = max(candidates, key=os.path.getmtime)
            emit({"type": "complete", "path": actual})
        else:
            emit_error(f"Download completed but file not found: {output_path}")


# ── CLI ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="EzYTDownloader yt-dlp backend")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--info",     metavar="URL")
    group.add_argument("--download", metavar="URL")

    parser.add_argument("--format", metavar="FMT")
    parser.add_argument("--output", metavar="PATH")
    parser.add_argument("--merge-audio", action="store_true")

    args = parser.parse_args()

    if args.info:
        fetch_info(args.info)
    elif args.download:
        if not args.format:
            emit_error("--format is required with --download")
            sys.exit(1)
        if not args.output:
            emit_error("--output is required with --download")
            sys.exit(1)
        download_stream(args.download, args.format, args.output,
                        merge_audio=args.merge_audio)


if __name__ == "__main__":
    main()
