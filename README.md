# subembed

A command-line tool for batch embedding subtitles into video files using ffmpeg.

## Requirements

- C++17 compiler (g++ or clang++)
- ffmpeg installed and available in PATH

## Build

```bash
make
```

## Usage

```bash
./subembed -v <video_dir> -s <subtitle_dir> [-o <output_dir>] [-n <output_template>]
```

### Options

- `-v, --input-video DIR` - Directory containing video files (required)
- `-s, --input-sub DIR` - Directory containing subtitle files (required)
- `-o, --output DIR` - Output directory (default: ./)
- `-n, --output-name NAME` - Output filename template (default: output-[index].mp4)
- `-h, --help` - Show help message

The `[index]` placeholder in the output name will be replaced with the episode number.

### Example

```bash
./subembed -v ./videos -s ./subtitles -o ./output -n "episode-[index].mp4"
```

## How it works

The tool scans both directories for video and subtitle files, extracts numeric indices from filenames, and matches them together. It then runs ffmpeg for each pair to embed the subtitles into the video file.

Supported video formats: mp4, mkv, avi, mov, m4v
Supported subtitle formats: vtt, srt, ass, ssa

## Install

```bash
sudo make install
```

This installs the binary to /usr/local/bin.
