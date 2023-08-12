#include <render/image.h>
#include <spng.h>
#include <assert.h>
#include <fstream>
#include <iterator>
#include <endianness.h>

using namespace render;

std::vector<char> render::load_png_rgb8(const std::string& path, unsigned int& width, unsigned int& height) {
    std::ifstream file{path, std::ios::in | std::ios::binary};

    if (!file.good()) {
        throw std::runtime_error(std::string("error opening file: ") + path);
    }

    std::vector<char> png_data{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    if (png_data.size() < 21) {
        throw std::runtime_error(std::string("invalid png: ") + path);
    }

    width = ntohl(*(unsigned int*)&png_data[16]);
    height = ntohl(*(unsigned int*)&png_data[20]);

    spng_ctx* ctx = spng_ctx_new(0);
    assert(ctx != NULL);

    int ret = spng_set_png_buffer(ctx, png_data.data(), png_data.size());
    if (ret != 0) {
        throw std::runtime_error(std::string("error in spng_set_png_buffer: ") + spng_strerror(ret) + ", " + path);
    }

    size_t out_size;
    ret = spng_decoded_image_size(ctx, SPNG_FMT_RGB8, &out_size);
    if (ret != 0) {
        throw std::runtime_error(std::string("error in spng_decoded_image_size: ") + spng_strerror(ret) + ", " + path);
    }

    std::vector<char> out(out_size);

    ret = spng_decode_image(ctx, out.data(), out_size, SPNG_FMT_RGB8, 0);
    if (ret != 0) {
        throw std::runtime_error(std::string("error in spng_decode_image for: ") + spng_strerror(ret) + ", " + path);
    }

    spng_ctx_free(ctx);

    return out;
}