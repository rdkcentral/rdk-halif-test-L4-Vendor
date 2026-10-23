## Overview
This document defines a comprehensive media playback validation test cases for L4 with validation methods and metrics used for validation.
The objective of these test cases is to verify correct media pipeline behavior, decoder functionality, rendering, synchronization, and playback stability during normal playback, pause& resume, forward seek, backward seek and trickmodes across supported stream formats and codec combinations.

Additional test cases will be added in future revisions of this document to extend coverage for more playback scenarios.

This test plan is structured around primary use cases to ensure comprehensive validation of the media pipeline:

## 1.1 Normal AV playback
**Purpose:** 
Validate complete decoder lifecycle including open/start/decode/queue operations, callback handling, stop/flush/EOS sequences, and basic audio-video synchronization.

| TC ID                 |   Testcase Description            | Video Codec | Audio Codec | Stream | Expected Behaviour | Validation caps | Validation steps | Metrics | Soc dependency |
| ----------------------|   ------------------------------  | ----------- | ----------- | ------ |------------------- | --------------- | ---------------- | ------- | --------------- |
| VTS_L4_H264_AC3_1001  | Verify H.264/AC3 playback by validating decoder lifecycle, buffer flow, A/V synchronization, callback handling, and correct EOS processing throughout the playback session.| H264 | AC3 | vts_h264_1080p_sdr_60.0f_BT709_yuv420p_8M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4 | Playback should happen without any  decoding errors mainting proper A/V sync and complets with out any errors | "Video Caps: video/x-h264 alignment=au<br> width=(int)1920,<br> height=(int)1080,<br> framerate=(fraction)60/1,<br>Audio Caps:<br>audio/x-ac3<br>framed=(boolean)true,<br>rate=(int)48000,<br>channels=(int)6" | "Decoder life cycle, playback transition state, buffer flow, Stop/Flush/EOS and GST_MESSAGE can be validated thorugh gst-validate-1.0.<br>1) Check that the pipeline state changes to playing and proper A/V sync and buffer_depth is maintained.<br>2) Monitor the GST_MESSAGE events are triggered properly during EOS and playback stop<br>3) Trigger FLUSH event manually and montor for GST_EVENT_FLUSH_START and STOP is triggered correctly also ensure all the queue buffers are drained correctly.<br>4) verify the corresponding call backs are triggered during first A/V frame rendered, decoder error and buffer underrun <br>Ensure minimal A/V frame drops<br>6) Detect video frame freeze and unintentional audio silence" | "Metrics:<br>Decoder lifecycle:<br>First-frame-presented: event - PTS(ms), Video Decode Errors: error count(int), PTS(ms), vendor-error-code, Audio decode errors: error count(int), PTS(ms), vendor-error-code, decoder_latency_ms: latency_ms(int64), render_latency_ms: latency_ms(int64), state: event(state)<br>Buffer status:Audio underflow: event, underflow count(int), Video underflow: event, underflow count(int), underflow_duration: duration_ms(int64), number_of_frames_decoded:frame_ count(int64), number_of_frames_received: frame_count(int64), number_of_frames_presented: frame_count(int64), number_of_frames_dropped: frame_dropped_count(int64), number_of_frames_dropped_frc: frame_dropped_count_frc(int64) ,number_of_frames_dropped_late_pts: frame_dropped_count_late_pts(int64), number_of_frames_repeated_frc: frame_repeated_count_frc(int64), buffer_depth: numner_of_buffers_queued(int64), video_fame_freeze:Duration(ms),  freeze_duration:Duration(ms) freeze_max_duration: Duration(ms)<br>A/V sync: av_ sync_offset: offset_ms(int64), av_sync_max_abs_offset:  abs_offset_ms(int64) ,av_resync_count: resync_count(int64),audio_pts_gap: duration(ms), PTS(ms),audio_silence: duration(ms), audio_pts_discontinuity:  duration(ms), audio_pts_error: pts_ms | Vendor_decoder_error_codes<br>number_of_frames_decoded<br>number_of_frames_received <br>number_of_frames_presented<br>number_of_frames_dropped<br>number_of_frames_dropped_frc<br>number_of_frames_dropped_late_pts<br>number_of_frames_repeated_frc<br>current_audio_pts <br>current_video_pts"|
| |
 









