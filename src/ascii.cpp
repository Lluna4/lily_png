#include "ascii.h"

#include "convert.h"
#include <math.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

void lily_png::convert_to_ascii(image &src, file_reader::buffer<char> &dest)
{
	std::string chars = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";
	image intermediate_img(src.meta);
	winsize terminal_size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
	intermediate_img.meta.height = terminal_size.ws_row - 1;
	std::println("{} {}", src.meta.height, src.meta.width);
	float height_ratio = (float)src.meta.height/(float)src.meta.width;
	intermediate_img.meta.width = (terminal_size.ws_row * height_ratio) * 2.25f;
	if (intermediate_img.meta.width > terminal_size.ws_col)
	{
		intermediate_img.meta.width = terminal_size.ws_col;
		float width_ratio = (float)src.meta.width/(float)src.meta.height;
		intermediate_img.meta.height = terminal_size.ws_col * width_ratio * 0.48f;
	}

	auto ret = src.resize_image(intermediate_img);
	if (!ret)
	{
		std::println("Error! {}", ret.error());
		return ;
	}
	size_t resized_size = ret.value();
	size_t reference_size = strlen("\033[38;2;255;255;255ma\033[0m");
	dest.allocate(resized_size + (resized_size * intermediate_img.meta.width * intermediate_img.meta.height));
	size_t dest_index = 0;
	size_t byte_size = (intermediate_img.meta.bit_depth + 7)/8;
	size_t lines = 0;
	for (int y = 0; y < intermediate_img.meta.height; y++)
	{
		for (int x = 0; x < intermediate_img.meta.width; x++)
		{
			unsigned char *pxl = intermediate_img[y, x];
			color_rgb tmp{};
			tmp.r = pxl[0];
			tmp.g = pxl[byte_size];
			tmp.b = pxl[byte_size * 2];
			double luminance = 0.299 * tmp.r + 0.587 * tmp.g + 0.114 * tmp.b;
			double luminance_normalized = luminance/255.0f;
			int index = static_cast<int>(luminance_normalized * (chars.length() - 1));

			std::string format = std::format("\033[38;2;{};{};{}m{}\033[0m", tmp.r, tmp.g, tmp.b, chars[index]);
			memcpy(&dest.data[dest_index], format.c_str(), format.size());
			dest_index += format.size();
			lines++;
			if (lines == intermediate_img.meta.width)
			{
				dest.data[dest_index] = '\n';
				dest_index++;
				lines = 0;
			}
		}
	}
}
