## Instruction

### Configuration

#### Align the answers with your test code

This test set assume that the input YUV frames are built in the test code. Change shell script variable `yuv` in `_config.sh`. If new input set is used, append your MD5 answers for the new input YUV frames in `_config.sh`.
If you'd like to test the performance, put YUV data into /tmp/ or use file cache for faster data filling.

#### Other Settings

Review settings in `_config.sh` to set the name of out_path. Refer description in `_config.sh` for the detail.

### Test

- Change  directory to script test case. Make sure that the shell could execute `feature_venc` or `venc` without path.
- There are 3 ways to test with this script set.
  - **Full Report**: `./test_i2_venc.sh` to test **all** test cases with summary information.
  - **Single Report**: `./test_i2_venc.sh i2_venc_test02.sh ` to test only test case 2 **with** summary information.
  - **Single Test**: `./i2_venc_test02.sh` to test only test case 2 **without** summary information.
