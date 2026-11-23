# L4 stream mapping: test cases to Polaris stream paths

Source: `docs/L4_Test_Cases.md` (stream column) and the Polaris `streams_config` files. Generated for review by the Sky team.

Status meanings:
- **Exact**: the stream name in the test case document matches a Polaris stream. The Polaris relative path is the download path under `stream_download_path`.
- **Not in Polaris**: the name is not in the Polaris list. Sky must confirm the correct file name or the path.

Totals: 85 test cases, 38 exact, 47 not in Polaris.

| Test case | Section | Proposed suite | Stream (test case document) | Polaris relative path | Status | Closest Polaris name |
|---|---|---|---|---|---|---|
| VTS_L4_H264_AC3_1001 | 1.1 Normal AV playback | NormalPlayback | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_8M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4 | Audio_Video/HD/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_8M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4 | Exact |  |
| VTS_L4_H265_EAC3_1001 | 1.1 Normal AV playback | NormalPlayback | vts_h265_1080i_sdr_60.0f_BT709_yuv420p_8M_eac3_48k_6ch_512k_a1875_v3599_60sec.mp4 |  | Not in Polaris | vts_h265_1080i_sdr_60.0f_BT709_yuv420p_8M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
| VTS_L4_MPEG2_AAC_1001 | 1.1 Normal AV playback | NormalPlayback | vts_mpeg2_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_256k_a2814_v3600_60sec.mp4 | Audio_Video/HD/vts_mpeg2_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_256k_a2814_v3600_60sec.mp4 | Exact |  |
| VTS_L4_VP9_AAC_1001 | 1.1 Normal AV playback | NormalPlayback | vts_vp9_1080p_sdr_60.0f_BT709_yuv420p_15M_aac_48k_2ch_512k_a2814_v3600_60sec.mp4 | Audio_Video/HD/vts_vp9_1080p_sdr_60.0f_BT709_yuv420p_15M_aac_48k_2ch_512k_a2814_v3600_60sec.mp4 | Exact |  |
| VTS_L4_AV1_AAC_1001 | 1.1 Normal AV playback | NormalPlayback | vts_av1_1080p_sdr_30.0f_BT709_yuv420p_10M_aac_48k_2ch_256k_a2814_v1800_60sec.mp4 | Audio_Video/HD/vts_av1_1080p_sdr_30.0f_BT709_yuv420p_10M_aac_48k_2ch_256k_a2814_v1800_60sec.mp4 | Exact |  |
| VTS_L4_H264_AC3_1001_01 | 1.1 Normal AV playback | NormalPlayback | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_8M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4 | Audio_Video/HD/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_8M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4 | Exact |  |
| VTS_L4_H265_EAC3_1001_01 | 1.1 Normal AV playback | NormalPlayback | vts_h265_1080i_sdr_60.0f_BT709_yuv420p_8M_eac3_48k_6ch_512k_a1875_v3599_60sec.mp4 |  | Not in Polaris | vts_h265_1080i_sdr_60.0f_BT709_yuv420p_8M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
| VTS_L4_AAC_1201 | 1.2 Audio Only | NormalPlayback | vts_aac_11.025k_2ch_64k_a647_v0_60sec.mp4 | Audio/vts_aac_11.025k_2ch_64k_a647_v0_60sec.mp4 | Exact |  |
| VTS_L4_AC3_1201 | 1.2 Audio Only | NormalPlayback | vts_ac3_32k_2ch_128k_a1250_v0_60sec.mp4 | Audio/vts_ac3_32k_2ch_128k_a1250_v0_60sec.mp4 | Exact |  |
| VTS_L4_EAC3_1201 | 1.2 Audio Only | NormalPlayback | vts_eac3_32k_3ch_128k_a1250_v0_60sec.mp4 | Audio/vts_eac3_32k_3ch_128k_a1250_v0_60sec.mp4 | Exact |  |
| VTS_L4_EAC3_ATMOS_1201 | 1.2 Audio Only | NormalPlayback | vts_eac3_atmos_32k_2ch_128k_a1250_v0_60sec.mp4 |  | Not in Polaris | vts_ac3_32k_2ch_128k_a1250_v0_60sec.mp4 |
| VTS_L4_PCM_1201 | 1.2 Audio Only | NormalPlayback | vts_pcm_44.1k_2ch_a2584_v0_60sec.mp4 | Audio/vts_pcm_44.1k_2ch_a2584_v0_60sec.mp4 | Exact |  |
| VTS_L4_OPUS_1201 | 1.2 Audio Only | NormalPlayback | vts_opus_12k_2ch_128k_a3001_v0_61sec.webm |  | Not in Polaris | vts_mp3_16k_2ch_128k_a1669_v0_60sec.mp4 |
| VTS_L4_VORBIS_1201 | 1.2 Audio Only | NormalPlayback | vts_vorbis_44.1k_2ch_128k_a2586_v0_61sec.webm |  | Not in Polaris |  |
| VTS_L4_FLAC_1201 | 1.2 Audio Only | NormalPlayback | vts_flac_44.1k_2ch_a575_v0_60sec.mp4 | Audio/vts_flac_44.1k_2ch_a575_v0_60sec.mp4 | Exact |  |
| VTS_L4_MP3_1201 | 1.2 Audio Only | NormalPlayback | vts_mp3_16k_2ch_128k_a1669_v0_60sec.mp4 | Audio/vts_mp3_16k_2ch_128k_a1669_v0_60sec.mp4 | Exact |  |
| VTS_L4_H264_1501 | 1.3 Video only | NormalPlayback | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Video/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_HEVC_1501 | 1.3 Video only | NormalPlayback | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v1440_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v14400_600sec.mp4 |
| VTS_L4_HEVC_1502 | 1.3 Video only | NormalPlayback | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Video/vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_VP9_1501 | 1.3 Video only | NormalPlayback | vts_vp9_2160p_sdr_60.0f_BT709_yuv420p_15M_a0_v3600_60sec.webm |  | Not in Polaris | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 |
| VTS_L4_AV1_1501 | 1.3 Video only | NormalPlayback | vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Video/vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_H264_AAC_7001 | 2.1 Audio + Video Combined | PauseResume | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 | Exact |  |
| VTS_L4_H264_AC3_7001 | 2.1 Audio + Video Combined | PauseResume | vts_h264_1080i_sdr_29.97f_BT709_yuv420p_5M_ac3_48k_6ch_256k_a1875_v1797_60sec.mp4 |  | Not in Polaris | vts_h264_1080i_sdr_29.97f_BT709_yuv420p_5M_ac3_48k_6ch_256k_a1875_v1799_61sec.mp4 |
| VTS_L4_H265_EAC3_7001 | 2.1 Audio + Video Combined | PauseResume | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_18M_eac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_18M_eac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Exact |  |
| VTS_L4_VP9_OPUS_7001 | 2.1 Audio + Video Combined | PauseResume | vts_vp9_2160p_sdr_60.0f_BT2020_yuv420p_18M_opus_48k_8ch_256k_a30001_v36000_601sec.webm |  | Not in Polaris | vts_vp9_1080p_sdr_60.0f_BT709_yuv420p_15M_aac_48k_2ch_512k_a2814_v3600_60sec.mp4 |
| VTS_L4_AV1_AC3_7001 | 2.1 Audio + Video Combined | PauseResume | vts_av1_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_av1_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Exact |  |
| VTS_L4_AAC_1301 | 2.2 Audio Only | PauseResume | vts_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_AC3_1301 | 2.2 Audio Only | PauseResume | vts_ac3_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_EAC3_1301 | 2.2 Audio Only | PauseResume | vts_eac3_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_H264_1601 | 2.3 Video Only | PauseResume | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Video/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_HEVC_1601 | 2.3 Video Only | PauseResume | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v1440_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v14400_600sec.mp4 |
| VTS_L4_HEVC_1602 | 2.3 Video Only | PauseResume | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Video/vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_VP9_1601 | 2.3 Video Only | PauseResume | vts_vp9_2160p_sdr_60.0f_BT709_yuv420p_15M_a0_v3600_60sec.webm |  | Not in Polaris | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 |
| VTS_L4_AV1_1601 | 2.3 Video Only | PauseResume | vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Video/vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_H264_AAC_7001_01 | 3.1 Audio + Video Combined | TrickModes | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 | Exact |  |
| VTS_L4_H264_AC3_7001_01 | 3.1 Audio + Video Combined | TrickModes | vts_h264_1080i_sdr_29.97f_BT709_yuv420p_5M_ac3_48k_6ch_256k_a1875_v1797_60sec.mp4 |  | Not in Polaris | vts_h264_1080i_sdr_29.97f_BT709_yuv420p_5M_ac3_48k_6ch_256k_a1875_v1799_61sec.mp4 |
| VTS_L4_H265_EAC3_7001_01 | 3.1 Audio + Video Combined | TrickModes | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_18M_eac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_18M_eac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Exact |  |
| VTS_L4_VP9_OPUS_7001_01 | 3.1 Audio + Video Combined | TrickModes | vts_vp9_2160p_sdr_60.0f_BT2020_yuv420p_18M_opus_48k_8ch_256k_a30001_v36000_601sec.webm |  | Not in Polaris | vts_vp9_1080p_sdr_60.0f_BT709_yuv420p_15M_aac_48k_2ch_512k_a2814_v3600_60sec.mp4 |
| VTS_L4_AV1_AC3_7001_01 | 3.1 Audio + Video Combined | TrickModes | vts_av1_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_av1_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Exact |  |
| VTS_L4_AAC_1301_01 | 3.2 Audio Only | TrickModes | vts_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_AC3_1301_01 | 3.2 Audio Only | TrickModes | vts_ac3_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_EAC3_1301_01 | 3.2 Audio Only | TrickModes | vts_eac3_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_H264_1601_01 | 3.3 Video Only | TrickModes | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Video/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_HEVC_1601_01 | 3.3 Video Only | TrickModes | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v1440_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v14400_600sec.mp4 |
| VTS_L4_HEVC_1602_01 | 3.3 Video Only | TrickModes | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Video/vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_VP9_1601_01 | 3.3 Video Only | TrickModes | vts_vp9_2160p_sdr_60.0f_BT709_yuv420p_15M_a0_v3600_60sec.webm |  | Not in Polaris | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 |
| VTS_L4_AV1_1601_01 | 3.3 Video Only | TrickModes | vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Video/vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_H264_AAC_7001_02 | 4.1 Audio + Video (combined) | Seek | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 | Exact |  |
| VTS_L4_H264_AC3_7001_02 | 4.1 Audio + Video (combined) | Seek | vts_h264_1080i_sdr_29.97f_BT709_yuv420p_5M_ac3_48k_6ch_256k_a1875_v1797_60sec.mp4 |  | Not in Polaris | vts_h264_1080i_sdr_29.97f_BT709_yuv420p_5M_ac3_48k_6ch_256k_a1875_v1799_61sec.mp4 |
| VTS_L4_H265_EAC3_7001_02 | 4.1 Audio + Video (combined) | Seek | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_18M_eac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_18M_eac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Exact |  |
| VTS_L4_VP9_OPUS_7001_02 | 4.1 Audio + Video (combined) | Seek | vts_vp9_2160p_sdr_60.0f_BT2020_yuv420p_18M_opus_48k_8ch_256k_a30001_v36000_601sec.webm |  | Not in Polaris | vts_vp9_1080p_sdr_60.0f_BT709_yuv420p_15M_aac_48k_2ch_512k_a2814_v3600_60sec.mp4 |
| VTS_L4_AV1_AC3_7001_02 | 4.1 Audio + Video (combined) | Seek | vts_av1_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_av1_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Exact |  |
| VTS_L4_AAC_1301_02 | 4.2 Audio only | Seek | vts_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_AC3_1301_02 | 4.2 Audio only | Seek | vts_ac3_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_EAC3_1301_02 | 4.2 Audio only | Seek | vts_eac3_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_H264_1601_02 | 4.3 Video only | Seek | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Video/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_HEVC_1601_02 | 4.3 Video only | Seek | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v1440_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v14400_600sec.mp4 |
| VTS_L4_HEVC_1602_02 | 4.3 Video only | Seek | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Video/vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_VP9_1601_02 | 4.3 Video only | Seek | vts_vp9_2160p_sdr_60.0f_BT709_yuv420p_15M_a0_v3600_60sec.webm |  | Not in Polaris | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 |
| VTS_L4_AV1_1601_02 | 4.3 Video only | Seek | vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Video/vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_H264_AAC_7001_03 | 5.1 Audio + Video (combined) | Seek | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_20M_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 | Exact |  |
| VTS_L4_H264_AC3_7001_03 | 5.1 Audio + Video (combined) | Seek | vts_h264_1080i_sdr_29.97f_BT709_yuv420p_5M_ac3_48k_6ch_256k_a1875_v1797_60sec.mp4 |  | Not in Polaris | vts_h264_1080i_sdr_29.97f_BT709_yuv420p_5M_ac3_48k_6ch_256k_a1875_v1799_61sec.mp4 |
| VTS_L4_H265_EAC3_7001_03 | 5.1 Audio + Video (combined) | Seek | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_18M_eac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_18M_eac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Exact |  |
| VTS_L4_VP9_OPUS_7001_03 | 5.1 Audio + Video (combined) | Seek | vts_vp9_2160p_sdr_60.0f_BT2020_yuv420p_18M_opus_48k_8ch_256k_a30001_v36000_601sec.webm |  | Not in Polaris | vts_vp9_1080p_sdr_60.0f_BT709_yuv420p_15M_aac_48k_2ch_512k_a2814_v3600_60sec.mp4 |
| VTS_L4_AV1_AC3_7001_03 | 5.1 Audio + Video (combined) | Seek | vts_av1_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Audio_Video/Long_Duration/Trick_Modes/vts_av1_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a18750_v36000_600sec.mp4 | Exact |  |
| VTS_L4_AAC_1301_03 | 5.2 Audio only | Seek | vts_aac_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_AC3_1301_03 | 5.2 Audio only | Seek | vts_ac3_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_EAC3_1301_03 | 5.2 Audio only | Seek | vts_eac3_48k_6ch_512k_a28126_v36000_600sec.mp4 |  | Not in Polaris | vts_aac_48k_6ch_512k_a28126_v0_600sec.mp4 |
| VTS_L4_H264_1601_03 | 5.3 Video only | Seek | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Video/vts_h264_1080p_sdr_60.0f_BT709_yuv420p_12M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_HEVC_1601_03 | 5.3 Video only | Seek | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v1440_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_24.0f_BT2020_yuv420p_25M_a0_v14400_600sec.mp4 |
| VTS_L4_HEVC_1602_03 | 5.3 Video only | Seek | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Video/vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_VP9_1601_03 | 5.3 Video only | Seek | vts_vp9_2160p_sdr_60.0f_BT709_yuv420p_15M_a0_v3600_60sec.webm |  | Not in Polaris | vts_h265_1080p_sdr_60.0f_BT709_yuv420p_5M_a0_v3600_60sec.mp4 |
| VTS_L4_AV1_1601_03 | 5.3 Video only | Seek | vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Video/vts_av1_2160p_hdr10_60.0f_BT2020_yuv420p_15M_a0_v3600_60sec.mp4 | Exact |  |
| VTS_L4_H264_AAC_6001_01 | 6.1 Low-to-High Bitrate | AdaptiveBitrate | vts_h264_abr_multi_bitrate_sdr_BT709_yuv420p_aac_48k_2ch_256k_300sec.mp4 |  | Not in Polaris |  |
| VTS_L4_H265_EAC3_6001_01 | 6.1 Low-to-High Bitrate | AdaptiveBitrate | vts_h265_abr_multi_bitrate_hdr10_BT2020_yuv420p_eac3_48k_6ch_512k_300sec.mp4 |  | Not in Polaris |  |
| VTS_L4_VP9_OPUS_6001_01 | 6.1 Low-to-High Bitrate | AdaptiveBitrate | vts_vp9_abr_multi_bitrate_sdr_BT709_yuv420p_opus_48k_2ch_256k_300sec.webm |  | Not in Polaris |  |
| VTS_L4_H264_AAC_6002_02 | 6.2 High-to-Low Bitrate | AdaptiveBitrate | vts_h264_abr_multi_bitrate_2k_BT709_yuv420p_aac_48k_2ch_256k_300sec.mp4 |  | Not in Polaris |  |
| VTS_L4_H265_EAC3_6002_02 | 6.2 High-to-Low Bitrate | AdaptiveBitrate | vts_h265_abr_multi_bitrate_2k_BT709_yuv420p_eac3_48k_6ch_512k_300sec.mp4 |  | Not in Polaris |  |
| VTS_L4_HDR10_2001 | 7.0 Metadata Propagation | MetadataPropagation | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_60M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_20M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
| VTS_L4_HDR10P_2001 | 7.0 Metadata Propagation | MetadataPropagation | vts_h265_2160p_hdr10plus_60.0f_BT2020_yuv420p_60M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_20M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
| VTS_L4_HLG_2001 | 7.0 Metadata Propagation | MetadataPropagation | vts_h265_2160p_hlg_60.0f_BT2020_yuv420p_60M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
| VTS_L4_SDR_2001 | 7.0 Metadata Propagation | MetadataPropagation | vts_vp9_2160p_sdr_60.0f_BT2020_yuv420p_20M_opus_48k_6ch_512k_a3001_v3600_61sec.webm |  | Not in Polaris | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_20M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
| VTS_L4_DVISION_2001 | 7.0 Metadata Propagation | MetadataPropagation | vts_h265_2160p_dolby_vision_60.0f_BT2020_yuv420p_20M_eac3_48k_6ch_512k_a3001_v3600_61sec.mp4 |  | Not in Polaris | vts_h265_2160p_hlg_60.0f_BT2020_yuv420p_20M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
| VTS_L4_META_FIELDS_2001 | 7.0 Metadata Propagation | MetadataPropagation | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_60M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_20M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
| VTS_L4_META_CONSISTENCY_2001 | 7.0 Metadata Propagation | MetadataPropagation | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_60M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |  | Not in Polaris | vts_h265_2160p_hdr10_60.0f_BT2020_yuv420p_20M_eac3_48k_6ch_512k_a1875_v3600_60sec.mp4 |
