# PLAY950 0.13.4

PLAY950 0.13.4 corrects VST3 MIDI-channel advertisement and removes audible
discontinuities when individual-output samples are replaced or stopped.

## MIDI

- Advertises all 16 MIDI channels on the VST3 event input, allowing hosts to
  deliver channelised notes to P9 keygroups instead of exposing only channel 1.
- Preserves Omni and keygroup-channel reception, Basic Channel offsets,
  channel-isolated pitch bend and channel-isolated note release.

## Sample endings and retriggers

- Starts a new Mono (01-08) voice immediately while retiring the displaced
  post-filter voice through a short 2.5 ms smooth tail.
- Gives amplitude-envelope release value 0 a 10 ms smooth post-filter
  retirement instead of stopping the voice at a discontinuity.
- Keeps programmed release values 1-99 unchanged.
- Uses fixed retirement storage on the audio thread, with no dynamic allocation,
  and retains the global eight-voice logical polyphony limit.

## Verification

- Verified the release-0 behaviour with a dedicated regression covering the
  full 10 ms retirement and logical Note Off state.
- Universal `arm64`/`x86_64` Release VST3 and bundled AKAI Util builds succeeded.
- All 7 CTest targets passed.
- Steinberg validator passed all 47 tests.
- Strict code-signature verification passed.
