## Overview
This document defines a comprehensive media playback validation test cases for L4 with validation methods and metrics used for validation.
The objective of these test cases is to verify correct media pipeline behavior, decoder functionality, rendering, synchronization, and playback stability during normal playback, pause& resume, forward seek, backward seek and trickmodes across supported stream formats and codec combinations.

Additional test cases will be added in future revisions of this document to extend coverage for more playback scenarios.

This test plan is structured around primary use cases to ensure comprehensive validation of the media pipeline:

##1.0 AV playback with non secure
#1.1 Normal AV playback
Purpose: Validate complete decoder lifecycle including open/start/decode/queue operations, callback handling, stop/flush/EOS sequences, and basic audio-video synchronization.


|TC ID   | Test case description | Video Codec | Audio Codec | Stream | Expected Behaviour | Validation caps | Validation steps | Metrics used for validation | Soc dependancy for metrics |
| -------- | -------- | -------- |
|VTS_L4_H264_AC3_1001  | Verify H.264/AC3 playback by validating decoder lifecycle, buffer flow, A/V synchronization, callback handling, and correct EOS processing throughout the playback session. |H264  |AC3  |vts_h264_1080p_sdr_60.0f_BT709_yuv420p_8M_ac3_48k_6ch_512k_a1875_v3600_60sec.mp4  |Playback should happen without any  decoding errors mainting proper A/V sync and complets with out any errors  |"Video Caps: video/x-h264 alignment=au 

width=(int)1920,
height=(int)1080,

framerate=(fraction)60/1,



Audio Caps:
audio/x-ac3

framed=(boolean)true,

rate=(int)48000,
 
channels=(int)6"  |"Decoder life cycle, playback transition state, buffer flow, Stop/Flush/EOS and GST_MESSAGE can be validated thorugh gst-validate-1.0.

1) Check that the pipeline state changes to playing and proper A/V sync and buffer_depth is maintained.

2) Monitor the GST_MESSAGE events are triggered properly during EOS and playback stop

3) Trigger FLUSH event manually and montor for GST_EVENT_FLUSH_START and STOP is triggered correctly also ensure all the queue buffers are drained correctly.

4) verify the corresponding call backs are triggered during first A/V frame rendered, decoder error and buffer underrun 

5) Ensure minimal A/V frame drops

6) Detect video frame freeze and unintentional audio silence"  | Metrics:
Decoder lifecycle:
First-frame-presented: event - PTS(ms)
Video Decode Errors: error count(int), PTS(ms), vendor-error-code
Audio decode errors: error count(int), PTS(ms), vendor-error-code
decoder_latency_ms: latency_ms(int64)
render_latency_ms: latency_ms(int64)
state: event(state) | Vendor_decoder_error_codes

number_of_frames_decoded

number_of_frames_received

number_of_frames_presented

number_of_frames_dropped

number_of_frames_dropped_frc

number_of_frames_dropped_late_pts

number_of_frames_repeated_frc

current_audio_pts

current_video_pts |

