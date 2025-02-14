#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <limits>
#include <string>
#include <thread>

#include <gui/Surface.h>
#include <media/ICrypto.h>
#include <media/MediaCodecBuffer.h>
#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaCodecConstants.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <utils/Timers.h>

int main(int argc, const char* argv[]) {
  if (argc != 8) {
    printf("Usage: %s <width> <height> <codec> <bitrate> <framerate> <min_qp> <max_qp> <input_path> <output_path>\n", argv[0]);
    return 0;
  }
  int width = std::atoi(argv[1]);
  int height = std::atoi(argv[2]);
  std::string codec = std::string("video/") + argv[3];  // hevc/avc
  int bitrate = std::atoi(argv[4]);
  int framerate = std::atoi(argv[5]);
  const char* input_path = argv[6];
  const char* output_path = argv[7];
  android::sp<android::AMessage> format = new android::AMessage();
  // Media Codec
  format->setInt32(KEY_WIDTH, width);
  format->setInt32(KEY_HEIGHT, height);
  format->setString(KEY_MIME, codec.c_str());
  //add debug code begin
  //format->setInt32(KEY_COLOR_FORMAT, OMX_COLOR_FormatYUV420SemiPlanar);    // NV12
  format->setInt32(KEY_COLOR_FORMAT, OMX_COLOR_FormatYUV420Planar);
  //add debug code end
  format->setInt32(KEY_BIT_RATE, bitrate);
  format->setInt32(KEY_MAX_BIT_RATE, bitrate);
  format->setInt32(KEY_BITRATE_MODE, BITRATE_MODE_CBR);
  format->setFloat(KEY_FRAME_RATE, framerate);
  format->setInt32(KEY_I_FRAME_INTERVAL, 360000);
  format->setInt32(KEY_MAX_B_FRAMES, 0);
  // format->setInt64(KEY_REPEAT_PREVIOUS_FRAME_AFTER, 10000000 / 30);

//add debug code begin
  if (bitrate == 3000000) {
	printf("bitrate == 3000000\n");
	format->setInt32("vendor.qti-ext-enc-initial-qp.qp-i-enable", 1);
	format->setInt32("vendor.qti-ext-enc-initial-qp.qp-i", 30);
	format->setInt32("video-qp-i-min", 30);
	format->setInt32("video-qp-i-max", 40);

	format->setInt32("vendor.qti-ext-enc-initial-qp.qp-p-enable", 1);
	format->setInt32("vendor.qti-ext-enc-initial-qp.qp-p", 30);
	format->setInt32("video-qp-p-min", 30);
	format->setInt32("video-qp-p-max", 40);
  } else if (bitrate == 2000000) {
	printf("bitrate == 2000000\n");
	format->setInt32("vendor.qti-ext-enc-initial-qp.qp-i-enable", 1);
	format->setInt32("vendor.qti-ext-enc-initial-qp.qp-i", 30);
	format->setInt32("video-qp-i-min", 30);
	format->setInt32("video-qp-i-max", 45);

	format->setInt32("vendor.qti-ext-enc-initial-qp.qp-p-enable", 1);
	format->setInt32("vendor.qti-ext-enc-initial-qp.qp-p", 30);
	format->setInt32("video-qp-p-min", 30);
	format->setInt32("video-qp-p-max", 45);
  }
//add debug code end

  if (codec == "video/avc") {
    format->setInt32(KEY_PROFILE, AVCProfileHigh);
    //add debug code begin
    //format->setInt32(KEY_LEVEL, AVCLevel5);
    format->setInt32(KEY_LEVEL, AVCLevel4);
    //add debug code end
  } else if (codec == "video/hevc") {
    format->setInt32(KEY_PROFILE, HEVCProfileMain);
  } else {
    assert(0);
  }
  // OMX
  format->setInt32("vendor.qti-ext-enc-low-latency.enable", 1);

  printf("%s", format->debugString().c_str());

  android::sp<android::ALooper> looper = new android::ALooper();
  looper->setName("yuv-encoder");
  looper->start();

  android::status_t status = android::NO_ERROR;

  auto encoder = android::MediaCodec::CreateByType(looper, codec.c_str(), true);
  status = encoder->configure(format, nullptr, nullptr, android::MediaCodec::CONFIGURE_FLAG_ENCODE);
  if (status != android::NO_ERROR) {
    printf("encoder->configure failed\n");
    return 0;
  }
  status = encoder->start();
  if (status != android::NO_ERROR) {
    printf("encoder->start failed\n");
    return 0;
  }

  android::Vector<android::sp<android::MediaCodecBuffer>> output_buffers{};
  android::Vector<android::sp<android::MediaCodecBuffer>> input_buffers{};

  status = encoder->getOutputBuffers(&output_buffers);
  if (status != android::NO_ERROR) {
    printf("encoder->getOutputBuffers failed\n");
    return 0;
  }
  status = encoder->getInputBuffers(&input_buffers);
  if (status != android::NO_ERROR) {
    printf("encoder->getInputBuffers failed\n");
    return 0;
  }

  std::ifstream input_file(input_path, std::ios::binary);
  std::ofstream output_file(output_path, std::ios::binary);

  int frame_number = 0;
  nsecs_t input_pts = systemTime(SYSTEM_TIME_MONOTONIC) / 1000;
  bool ended = false;
  while (true) {
    size_t buf_index = 0;
    size_t offset = 0;
    size_t size = 0;
    int64_t pts_us = 0;
    uint32_t flags = 0;
    if (!ended) {
      status = encoder->dequeueInputBuffer(&buf_index);
      switch (status) {
        case android::WOULD_BLOCK:
          break;
        case android::NO_ERROR: {
          if (input_file.eof()) {
            ended = true;
            break;
          }
          input_file.read(reinterpret_cast<char*>(input_buffers[buf_index]->data()), width * height * 3 / 2);
          status = encoder->queueInputBuffer(buf_index, 0, width * height * 3 / 2, input_pts, 0);
          input_pts += 1000000/framerate;
          if (status != android::NO_ERROR) {
            printf("encoder->queueInputBuffer failed: %d\n", status);
          }
          break;
        }
        default:
          printf("encoder->dequeueInputBuffer get status: %d\n", status);
          break;
      }
    }

    status = encoder->dequeueOutputBuffer(&buf_index, &offset, &size, &pts_us, &flags, 50000);
    switch (status) {
      case android::INFO_FORMAT_CHANGED:
      case android::WOULD_BLOCK:
        break;
      case android::NO_ERROR: {
        output_file.write(reinterpret_cast<char*>(output_buffers[buf_index]->data()),
                          output_buffers[buf_index]->size());
        output_file.flush();
        status = encoder->releaseOutputBuffer(buf_index);
        if (status != android::NO_ERROR) {
          printf("encoder->releaseOutputBuffer failed: %d\n", status);
        }
        if ((flags & android::MediaCodec::BUFFER_FLAG_CODECCONFIG) == 0) {
          printf("Frame number %d\n", ++frame_number);
        }
        break;
      }
      default:
        printf("encoder->dequeueOutputBuffer get status: %d\n", status);
        break;
    }

    if (ended && status == android::WOULD_BLOCK) {
      break;
    }
  }

  input_file.close();
  output_file.close();

  encoder->stop();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  encoder->release();

  return 0;
}
