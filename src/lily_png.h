#pragma once

#include "../file_reader/src/file_read.h"
#include <string>
#include <vector>
#include <format>
#include "utils.h"
#include "filter.h"
#include <zlib.h>

namespace lily_png
{
	struct color_rgb
	{
		unsigned char r = 0;
		unsigned char g = 0;
		unsigned char b = 0;
	};

	enum class png_error
	{
		file_doesnt_exist,
		read_failed,
		file_is_not_a_png
	};



	std::expected<metadata, png_error> read_png(const std::string &file_path, file_reader::buffer<unsigned char> &data);
}

template <>
struct std::formatter<lily_png::png_error>
{

	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(const lily_png::png_error& id, std::format_context& ctx) const
	{
		if (id == lily_png::png_error::file_doesnt_exist)
			return std::format_to(ctx.out(), "{}", "File doesn't exist");
		if (id == lily_png::png_error::read_failed)
			return std::format_to(ctx.out(), "{}", "Read failed");
		if (id == lily_png::png_error::file_is_not_a_png)
			return std::format_to(ctx.out(), "{}", "File is not a png");
		return std::format_to(ctx.out(), "{}", "Unkown error");
	}
};