// gesichtool - Face Extraction Tool
// Copyright (C) 2023 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <optional>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <string>
#include <vector>
#include <semaphore>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/opencv.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <opencv2/opencv.hpp>

#include "semaphore_guard.hpp"

std::vector<cv::Rect> detect_faces(cv::CascadeClassifier& face_cascade,
                                   cv::Mat const& image,
                                   std::optional<cv::Size> min_size,
                                   std::optional<cv::Size> max_size,
                                   int min_neighbors)
{
  std::vector<cv::Rect> faces;
  face_cascade.detectMultiScale(image, faces, 1.1, min_neighbors, 0,
                                min_size.value_or(cv::Size()),
                                max_size.value_or(cv::Size()));

  return faces;
}

void extract_faces(cv::Mat const& image, std::vector<cv::Rect> const& faces,
                   int image_idx,
                   std::filesystem::path const& output_directory,
                   cv::Size const& output_size)
{
  int face_idx = 0;
  for (cv::Rect const& face : faces)
  {
    // int const inflation = static_cast<int>(static_cast<double>(face.width) * 0.2);
    int const inflation = 0;

    cv::Rect enlarged_face(face.x - inflation,
                           face.y - inflation,
                           face.width + inflation * 2,
                           face.height + inflation * 2);

    if (enlarged_face.tl().x < 0 ||
        enlarged_face.tl().y < 0 ||
        enlarged_face.br().x >= image.cols ||
        enlarged_face.br().y >= image.rows)
    {
      fmt::print("  enlargement rejected\n");
      enlarged_face = face;
    }

    fmt::print("extracting face at: {} {} {} {} from {}x{}\n",
               enlarged_face.x,
               enlarged_face.y,
               enlarged_face.width,
               enlarged_face.height,
               image.cols, image.rows);

    cv::Mat face_image = image(enlarged_face);

    std::string const filename = output_directory / fmt::format("face{:03d}-{:03d}.jpg", image_idx, face_idx);

    cv::resize(face_image, face_image, output_size);

    cv::imwrite(filename, face_image);

    face_idx += 1;
  }
}

enum class Mode
{
  DLIB,
  OPENCV
};

struct Options
{
  Mode mode = Mode::DLIB;
  std::vector<std::filesystem::path> images = {};
  std::filesystem::path output_directory = {};
  cv::Size output_size = cv::Size(512, 512);
  std::optional<cv::Size> min_size = cv::Size(512, 512);
  std::optional<cv::Size> max_size = {};
  bool verbose = false;
  int min_neighbors = 3;
};

class ArgParseError : public std::runtime_error
{
public:
  ArgParseError(std::string const& message) :
    std::runtime_error(message) {}
};

void print_help()
{
  fmt::print(
    "Usage: gesichtool [OPTIONS] IMAGE... -o OUTDIR\n"
    "Extract faces from image files\n"
    "\n"
    "General Options:\n"
    "  -h, --help                Print this help\n"
    "  -v, --verbose             Be more verbose\n"
    "\n"
    "Face Detect Mode:\n"
    "  --dlib                    Use dlib face detection (default)\n"
    "  --opencv                  Use OpenCV face detection\n"
    "Face Detect Options:\n"
    "  -n, --min-neighbors INT   Higher values reduce false positives (default: 3)\n"
    "  --min-size WxH            Minimum sizes for detected faces\n"
    "  --max-size WxH            Maximum sizes for detected faces\n"
    "\n"
    "Output Options:\n"
    "  -o, --output DIR          Output directory\n"
    "  --size WxH         Rescale output images to WxH (default: 512x512)\n"
    );
}

cv::Size to_size(std::string const& text)
{
  cv::Size size;
  if (sscanf(text.c_str(), "%dx%d", &size.width, &size.height) != 2) {
    throw std::runtime_error(fmt::format("failed to read cv::Size from {}", text));
  }
  return size;
}

Options parse_args(std::vector<std::string> const& argv)
{
  Options opts;

  for (size_t argv_idx = 0; argv_idx < argv.size(); ++argv_idx)
  {
    std::string const& arg = argv[argv_idx];

    if (!arg.empty() && arg[0] == '-') {
      if (arg == "-o" || arg == "--option") {
        argv_idx += 1;
        if (argv_idx >= argv.size()) {
          throw ArgParseError(fmt::format("{} requires an argument", arg));
        }

        opts.output_directory = argv[argv_idx];
      }
      else if (arg == "-v" || arg == "--verbose") {
        opts.verbose = true;
      }
      else if (arg == "-h" || arg == "--help") {
        print_help();
        exit(EXIT_SUCCESS);
      }
      else if (arg == "-n" || arg == "--min-neighbors") {
        argv_idx += 1;
        if (argv_idx >= argv.size()) {
          throw ArgParseError(fmt::format("{} requires an argument", arg));
        }

        try {
          opts.min_neighbors = std::stoi(argv[argv_idx]);
        } catch (std::exception const& err) {
          std::throw_with_nested(ArgParseError(fmt::format("invalid value {} for argument", argv[argv_idx], arg)));
        }
      }
      else if (arg == "--min-size") {
        argv_idx += 1;
        if (argv_idx >= argv.size()) {
          throw ArgParseError(fmt::format("{} requires an argument", arg));
        }

        opts.min_size = to_size(argv[argv_idx]);
      }
      else if (arg == "--max-size") {
        argv_idx += 1;
        if (argv_idx >= argv.size()) {
          throw ArgParseError(fmt::format("{} requires an argument", arg));
        }

        opts.max_size = to_size(argv[argv_idx]);
      }
      else if (arg == "--size") {
        argv_idx += 1;
        if (argv_idx >= argv.size()) {
          throw ArgParseError(fmt::format("{} requires an argument", arg));
        }

        opts.output_size = to_size(argv[argv_idx]);
      }
      else if (arg == "--opencv") {
        opts.mode = Mode::OPENCV;
      }
      else if (arg == "--dlib") {
        opts.mode = Mode::DLIB;
      }
      else {
        throw ArgParseError(fmt::format("unknown argument {} given", arg));
      }
    }
    else
    {
      opts.images.emplace_back(arg);
    }
  }

  if (opts.images.empty()) {
    throw ArgParseError("no input images given");
  }

  if (opts.output_directory.empty()) {
    throw ArgParseError("no output directory given");
  }

  return opts;
}

void run_dlib(Options const& opts)
{
  fmt::print("running dlib face detection\n");

  std::counting_semaphore sem(std::thread::hardware_concurrency());
  std::vector<std::future<void>> futures;
  for (size_t images_idx = 0; images_idx < opts.images.size(); ++images_idx)
  {
    futures.push_back(std::async(std::launch::async, [&sem, images_idx, &opts]{
      SemaphoreGuard guard(sem);

      std::filesystem::path const& input_image_path = opts.images[images_idx];

      if (opts.verbose) {
        fmt::print("processing {}\n", input_image_path);
      }

      // not thread safe, so recreate it each thread
      dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();

      cv::Mat const image = cv::imread(input_image_path);
      cv::Mat gray;
      cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

      dlib::cv_image<unsigned char> const dlibImage(gray);

      std::vector<dlib::rectangle> const dlib_faces = detector(dlibImage);

      std::vector<cv::Rect> faces;
      for (auto const& rect : dlib_faces)
      {
        faces.emplace_back(static_cast<int>(rect.left()),
                           static_cast<int>(rect.top()),
                           static_cast<int>(rect.width()),
                           static_cast<int>(rect.height()));
      }

      extract_faces(image, faces,
                    images_idx,
                    opts.output_directory,
                    opts.output_size);
    }));
  }

  fmt::print("waiting for results\n");
  for (auto& future : futures) {
    future.get();
  }
}

void run_opencv(Options const& opts)
{
  fmt::print("running OpenCV face detection\n");

  std::string const cascade_file = cv::samples::findFile("haarcascades/haarcascade_frontalface_default.xml");

  std::counting_semaphore sem(std::thread::hardware_concurrency());
  std::vector<std::future<void>> futures;
  for (size_t images_idx = 0; images_idx < opts.images.size(); ++images_idx)
  {
    std::filesystem::path const& input_image_path = opts.images[images_idx];

    if (opts.verbose) {
      fmt::print("processing {}\n", input_image_path);
    }

    futures.push_back(std::async(std::launch::async, [&sem, images_idx, &input_image_path, &opts, &cascade_file]{
      SemaphoreGuard guard(sem);

      // CascadeClassifier is neither thread safe nor can it be copied
      cv::CascadeClassifier face_cascade;
      if (!face_cascade.load(cascade_file)) {
        throw std::runtime_error(fmt::format("failed to load {}", cascade_file));
      }

      cv::Mat image = cv::imread(input_image_path);
      if (image.empty()) {
        fmt::print(stderr, "error: failed to read image: {}\n", input_image_path);
      } else {
        std::vector<cv::Rect> faces = detect_faces(face_cascade, image,
                                                   opts.min_size,
                                                   opts.max_size,
                                                   opts.min_neighbors);
        extract_faces(image, faces,
                      images_idx,
                      opts.output_directory,
                      opts.output_size);

        fmt::print("  detected {}\n", faces.size());
      }
    }));
  }

  fmt::print("waiting for results\n");
  for (auto& future : futures) {
    future.get();
  }
}

void run(Options const& opts)
{
  std::filesystem::create_directory(opts.output_directory);

  switch (opts.mode)
  {
    case Mode::OPENCV:
      run_opencv(opts);
      break;

    case Mode::DLIB:
      run_dlib(opts);
      break;
  }
}

int main(int argc, char* argv[])
{
  try {
    std::vector<std::string> const args(argv + 1, argv + argc);

    Options const opts = parse_args(args);

    run(opts);

    return 0;
  } catch (std::exception const& err) {
    fmt::print(stderr, "error: {}\n", err.what());
  }
}

/* EOF */
