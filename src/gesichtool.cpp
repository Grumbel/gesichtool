#include <optional>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <string>
#include <vector>
#include <semaphore>

#include <fmt/format.h>
#include <fmt/std.h>
#include <opencv2/opencv.hpp>

#include "semaphore_guard.hpp"

void detect_and_extract_faces(cv::CascadeClassifier& face_cascade,
                              int image_idx, cv::Mat image,
                              std::optional<cv::Size> min_size,
                              std::optional<cv::Size> max_size,
                              int min_neighbors,
                              std::filesystem::path output_directory)
{
  std::vector<cv::Rect> faces;
  face_cascade.detectMultiScale(image, faces, 1.1, min_neighbors, 0,
                                min_size.value_or(cv::Size()),
                                max_size.value_or(cv::Size()));

  fmt::print("  detected {}\n", faces.size());

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

    fmt::print("extracting face at: {} {} {} {}\n",
               enlarged_face.x,
               enlarged_face.y,
               enlarged_face.width,
               enlarged_face.height);

    cv::Mat face_image = image(enlarged_face);

    std::string const filename = output_directory / fmt::format("face{:03d}-{:03d}.jpg", image_idx, face_idx);

    cv::resize(face_image, face_image, cv::Size(512, 512));

    cv::imwrite(filename, face_image);

    face_idx += 1;
  }
}

struct Options
{
  std::vector<std::filesystem::path> images = {};
  std::filesystem::path output_directory = {};
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
    "Options:\n"
    "  -h, --help                Print this help\n"
    "  -v, --verbose             Be more verbose\n"
    "  -o, --output DIR          Output directory\n"
    "  -n, --min-neighbors INT   Higher values reduce false positives (default: 3)\n"
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

void run(Options const& opts)
{
  std::filesystem::create_directory(opts.output_directory);

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
        detect_and_extract_faces(face_cascade, images_idx, std::move(image),
                                 opts.min_size,
                                 opts.max_size,
                                 opts.min_neighbors,
                                 opts.output_directory);
      }
    }));
  }

  fmt::print("waiting for results\n");
  for (auto& future : futures) {
    future.get();
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
