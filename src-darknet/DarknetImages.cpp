// DarkMark (C) 2019-2021 Stephane Charette <stephanecharette@gmail.com>

#include <random>
#include "DarkMark.hpp"
#include "json.hpp"
using json = nlohmann::json;


void dm::DarknetWnd::find_all_annotated_images(ThreadWithProgressWindow & progress_window, VStr & annotated_images, VStr & skipped_images, size_t & number_of_marks)
{
	double work_done = 0.0;
	double work_to_do = content.image_filenames.size() + 1.0;
	progress_window.setProgress(0.0);
	progress_window.setStatusMessage("Finding all images and annotations...");

	annotated_images.clear();
	skipped_images.clear();

	for (const auto & filename : content.image_filenames)
	{
		work_done ++;
		progress_window.setProgress(work_done / work_to_do);

		File f = File(filename).withFileExtension(".json");
		const size_t count = content.count_marks_in_json(f);
		if (count)
		{
			annotated_images.push_back(filename);
		}
		else
		{
			skipped_images.push_back(filename);
		}
		number_of_marks += count;
	}

	work_done = 0.0;
	work_to_do = skipped_images.size() + 1.0;
	progress_window.setProgress(0.0);
	progress_window.setStatusMessage("Listing skipped images...");

	std::random_shuffle(skipped_images.begin(), skipped_images.end());
	const std::string fn = File(info.project_dir).getChildFile("skipped_images.txt").getFullPathName().toStdString();
	std::ofstream fs_skipped(fn);
	for (const auto & image_filename : skipped_images)
	{
		work_done ++;
		progress_window.setProgress(work_done / work_to_do);

		fs_skipped << image_filename << std::endl;
	}

	return;
}


void dm::DarknetWnd::resize_images(ThreadWithProgressWindow & progress_window, const VStr & annotated_images, VStr & all_output_images, size_t & number_of_resized_images)
{
	double work_done = 0.0;
	double work_to_do = annotated_images.size() + 1.0;
	progress_window.setProgress(0.0);
	progress_window.setStatusMessage("Resizing images to " + std::to_string(info.image_width) + "x" + std::to_string(info.image_height) + "...");

	File dir = File(info.project_dir).getChildFile("darkmark_image_cache").getChildFile("resize");
	const std::string dir_name = dir.getFullPathName().toStdString();
	dir.createDirectory();
	if (dir.isDirectory() == false)
	{
		throw std::runtime_error("Failed to create directory " + dir_name + ".");
	}

	const cv::Size desired_image_size(info.image_width, info.image_height);

	std::ofstream resized_txt(dir_name + "/resized.txt");

	for (const auto original_image : annotated_images)
	{
		work_done ++;
		progress_window.setProgress(work_done / work_to_do);

		std::stringstream ss;
		ss << dir_name << "/" << std::setfill('0') << std::setw(8) << all_output_images.size();
		const std::string output_base_name = ss.str();
		const std::string output_image = output_base_name + ".jpg";
		const std::string output_label = output_base_name + ".txt";
		all_output_images.push_back(output_image);

		// first we create the resized image file
		cv::Mat mat = cv::imread(original_image);

		cv::Mat dst;
		if (mat.cols != desired_image_size.width and mat.rows != desired_image_size.height)
		{
			cv::resize(mat, dst, desired_image_size, cv::INTER_AREA);
			number_of_resized_images ++;
		}
		else
		{
			dst = mat;
		}

		resized_txt
			<< original_image
			<< " [" << mat.cols << "x" << mat.rows << "] -> "
			<< output_image
			<< " [" << dst.cols << "x" << dst.rows << "]"
			<< std::endl;

		cv::imwrite(output_image, dst, {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 75});

		// next we copy the annoations in the .txt file
		File txt = File(original_image).withFileExtension(".txt");
		const bool success = txt.copyFileTo(File(output_label));
		if (not success)
		{
			throw std::runtime_error("Failed to copy " + txt.getFullPathName().toStdString() + ".");
		}
	}

	return;
}


void dm::DarknetWnd::tile_images(ThreadWithProgressWindow & progress_window, const VStr & annotated_images, VStr & all_output_images, size_t & number_of_marks, size_t & number_of_tiles_created)
{
	double work_done = 0.0;
	double work_to_do = annotated_images.size() + 1.0;
	progress_window.setProgress(0.0);
	progress_window.setStatusMessage("Tiling images to " + std::to_string(info.image_width) + "x" + std::to_string(info.image_height) + "...");

	File dir = File(info.project_dir).getChildFile("darkmark_image_cache").getChildFile("tiles");
	const std::string dir_name = dir.getFullPathName().toStdString();
	dir.createDirectory();
	if (dir.isDirectory() == false)
	{
		throw std::runtime_error("Failed to create directory " + dir_name + ".");
	}

	std::ofstream tiles_txt(dir_name + "/tiles.txt");
	const cv::Size desired_tile_size(info.image_width, info.image_height);

	for (const auto original_image : annotated_images)
	{
		work_done ++;
		progress_window.setProgress(work_done / work_to_do);

		// first thing we'll do is read the annotations for this image
		json root = json::parse(File(original_image).withFileExtension(".json").loadFileAsString().toStdString());

		cv::Mat mat = cv::imread(original_image);

		const double horizontal_factor		= static_cast<double>(mat.cols) / static_cast<double>(desired_tile_size.width);
		const double vertical_factor		= static_cast<double>(mat.rows) / static_cast<double>(desired_tile_size.height);
		const size_t horizontal_tiles_count	= std::round(std::max(1.0, horizontal_factor	));
		const size_t vertical_tiles_count	= std::round(std::max(1.0, vertical_factor		));
		const double cell_width				= static_cast<double>(mat.cols) / static_cast<double>(horizontal_tiles_count);
		const double cell_height			= static_cast<double>(mat.rows) / static_cast<double>(vertical_tiles_count);

		tiles_txt
			<< original_image << " [" << mat.cols << "x" << mat.rows << "]"
			<< " -> [" << horizontal_tiles_count << "x" << vertical_tiles_count << "]"
			<< " -> [" << cell_width << "x" << cell_height << "]"
			<< std::endl;

		if (info.resize_images and horizontal_tiles_count == 1 and vertical_tiles_count == 1)
		{
			// this image only has 1 tile, and we already have it since "resize" is enabled, so skip to the next image
			tiles_txt << "-> skipped (single tile)" << std::endl;
			continue;
		}

//		Log(original_image + " (" + std::to_string(mat.cols) + "x" + std::to_string(mat.rows) + ") needs to be tiled as " + std::to_string(horizontal_tiles_count) + "x" + std::to_string(vertical_tiles_count) + " tiles each measuring " + std::to_string(desired_tile_size.width) + "x" + std::to_string(desired_tile_size.height));

		for (size_t y_idx = 0; y_idx < vertical_tiles_count; y_idx ++)
		{
			for (size_t x_idx = 0; x_idx < horizontal_tiles_count; x_idx ++)
			{
				int tile_x = std::round(cell_width	* static_cast<double>(x_idx));
				int tile_y = std::round(cell_height	* static_cast<double>(y_idx));
				int tile_w = std::round(cell_width);
				int tile_h = std::round(cell_height);
//				Log("-> old tile: x=" + std::to_string(tile_x) + " y=" + std::to_string(tile_y) + " w=" + std::to_string(tile_w) + " h=" + std::to_string(tile_h));

				// if a cell is smaller than our desired tile, then we can grab a few more pixels to fill out the tile and get it closer to the desired network size
				int delta = desired_tile_size.width - tile_w;
				tile_x -= delta / 2;
				tile_w += delta;

				// if we moved beyond the right border then move the X coordinate back
				if (tile_x + tile_w >= mat.cols)
				{
					tile_x = mat.cols - tile_w;
				}

				// if we moved beyond the *left* border, then reset to zero
				if (tile_x < 0)
				{
					tile_x = 0;
				}

				// make sure the cell width doesn't extend beyond the right border
				if (tile_x + tile_w >= mat.cols)
				{
					tile_w = (mat.cols - tile_x);
				}

				delta = desired_tile_size.height - tile_h;
				tile_y -= delta / 2;
				tile_h += delta;

				// if we moved beyond the bottom border then move the Y coordinate back
				if (tile_y + tile_h >= mat.rows)
				{
					tile_y = mat.rows - tile_h;
				}

				// if we moved beyond the *top* border, then reset to zero
				if (tile_y < 0)
				{
					tile_y = 0;
				}

				// make sure the cell width doesn't extend beyond the bottom border
				if (tile_y + tile_h >= mat.rows)
				{
					tile_h = (mat.rows - tile_y);
				}
//				Log("-> new tile: x=" + std::to_string(tile_x) + " y=" + std::to_string(tile_y) + " w=" + std::to_string(tile_w) + " h=" + std::to_string(tile_h));

				const cv::Rect tile_rect(tile_x, tile_y, tile_w, tile_h);
				cv::Mat tile = mat(tile_rect);

				std::stringstream ss;
				ss << dir_name << "/" << std::setfill('0') << std::setw(8) << all_output_images.size();
				const std::string output_base_name = ss.str();
				const std::string output_image = output_base_name + ".jpg";
				const std::string output_label = output_base_name + ".txt";

				cv::imwrite(output_image, tile, {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 75});
				all_output_images.push_back(output_image);

				// now re-create the .txt file with the appropriate annotations for this new tile
				//
				// we know our tile is from (tile_x, tile_y, tile_w, tile_h), so include any annotations within those bounds
				std::ofstream fs_txt(output_label);
				fs_txt << std::fixed << std::setprecision(10);

				size_t number_of_annotations = 0;
				for (auto j : root["mark"])
				{
					const cv::Rect annotation_rect(j["rect"]["int_x"], j["rect"]["int_y"], j["rect"]["int_w"], j["rect"]["int_h"]);
					const cv::Rect intersection = annotation_rect & tile_rect;
					if (intersection.area() > 0)
					{
						const int class_idx = j["class_idx"];
						int x = j["rect"]["int_x"];
						int y = j["rect"]["int_y"];
						int w = j["rect"]["int_w"];
						int h = j["rect"]["int_h"];

						if (x < tile_rect.x)
						{
							// X is beyond the left border, we need to move it to the right
							int delta = tile_rect.x - x;
							x += delta;
							w -= delta;
						}
						if (y < tile_rect.y)
						{
							int delta = tile_rect.y - y;
							y += delta;
							h -= delta;
						}
						if (x + w > tile_rect.x + tile_rect.width)
						{
							w = tile_rect.x + tile_rect.width - x;
						}
						if (y + h > tile_rect.y + tile_rect.height)
						{
							h = tile_rect.y + tile_rect.height - y;
						}

						// ignore extremely tiny slices
						if (w >= 10 and h >= 10)
						{
							// bring all the coordinates back down to zero
							x -= tile_rect.x;
							y -= tile_rect.y;

							const double normalized_w = static_cast<double>(w) / static_cast<double>(tile.cols);
							const double normalized_h = static_cast<double>(h) / static_cast<double>(tile.rows);
							const double normalized_x = static_cast<double>(x) / static_cast<double>(tile.cols) + normalized_w / 2.0;
							const double normalized_y = static_cast<double>(y) / static_cast<double>(tile.rows) + normalized_h / 2.0;
							fs_txt << class_idx << " " << normalized_x <<  " " << normalized_y << " " << normalized_w << " " << normalized_h << std::endl;
							number_of_annotations ++;
						}
					}
				}

				number_of_marks += number_of_annotations;
				number_of_tiles_created ++;

				tiles_txt
					<< "-> " << output_image
					<< " [" << tile.cols << "x" << tile.rows << "]"
					<< " [" << number_of_annotations << "/" << root["mark"].size() << "]"
					<< std::endl;
			}
		}
	}

	return;
}


void dm::DarknetWnd::random_zoom_images(ThreadWithProgressWindow & progress_window, const VStr & annotated_images, VStr & all_output_images, size_t & number_of_marks, size_t & number_of_zooms_created)
{
	double work_done = 0.0;
	double work_to_do = annotated_images.size() + 1.0;
	progress_window.setProgress(0.0);
	progress_window.setStatusMessage("Random image crop and zoom...");

	File dir = File(info.project_dir).getChildFile("darkmark_image_cache").getChildFile("zoom");
	const std::string dir_name = dir.getFullPathName().toStdString();
	dir.createDirectory();
	if (dir.isDirectory() == false)
	{
		throw std::runtime_error("Failed to create directory " + dir_name + ".");
	}

	std::ofstream zoom_txt(dir_name + "/zoom.txt");
	const cv::Size desired_size(info.image_width, info.image_height);

	/* Images must be larger than the final desired size for us to "zoom in".
	 * This variable describes the minimum size we need for us to work with the image.
	 */
	const cv::Size large_size(
			std::round(1.25f * desired_size.width),
			std::round(1.25f * desired_size.height));

	std::random_device rd;
	std::mt19937 rng(rd());

	for (const auto original_image : annotated_images)
	{
		work_done ++;
		progress_window.setProgress(work_done / work_to_do);

		cv::Mat original_mat = cv::imread(original_image);
		if (original_mat.cols < large_size.width or original_mat.rows < large_size.height)
		{
			zoom_txt
				<< original_image
				<< " [" << original_mat.cols << "x" << original_mat.rows << "]"
				<< " -> skipped (image too small)" << std::endl;
			continue;
		}

		// keep creating cropped/zoomed images as long as we're finding new parts of the image that we didn't previously cover
		std::vector<cv::Rect> all_previous_rectangles;
		size_t failed_consecutive_attempts = 0;

		while (failed_consecutive_attempts < 3)
		{
			/* The amount we're going to "zoom in" depends on exactly how big the image is compared to the final size.
			* This value is the "factor" by which we multiply the desired image size.  We need to make sure that both
			* the horizontal and vertical values can be satisfied.
			*/
			const float horizontal_factor	= static_cast<float>(original_mat.cols) / static_cast<float>(desired_size.width);
			const float vertical_factor		= static_cast<float>(original_mat.rows) / static_cast<float>(desired_size.height);
			const float min_factor			= std::min(horizontal_factor, vertical_factor);

			std::uniform_real_distribution<float> uni_f(1.0f, min_factor);
			const float factor = uni_f(rng);

			// This describes the size of the RoI we're going to carve out of the original image mat.
			const cv::Size size(
					std::round(factor * desired_size.width),
					std::round(factor * desired_size.height));

			// Now that we know the size, we can create the rectangle which is used to carve out the RoI.
			cv::Rect roi(cv::Point(0, 0), size);

			// Now figure out how much room remains outside of the RoI, and randomly choose some spacing to assing.
			const int delta_h = original_mat.cols - roi.width;
			const int delta_v = original_mat.rows - roi.height;
			std::uniform_int_distribution<int> uni_h(0, delta_h);
			std::uniform_int_distribution<int> uni_v(0, delta_v);
			roi.x = uni_h(rng);
			roi.y = uni_v(rng);

			// See if the middle point of this RoI was already covered by a previous rectangle.
			bool continue_crop_and_zoom = true;
			const cv::Point middle_point(roi.x + roi.width/2, roi.y + roi.height/2);
			for (const auto & r : all_previous_rectangles)
			{
				if (r.contains(middle_point))
				{
					continue_crop_and_zoom = false;
					break;
				}
			}

			if (continue_crop_and_zoom == false)
			{
				zoom_txt
					<< original_image
					<< " -> skipped RoI [x=" << roi.x << " y=" << roi.y << " w=" << roi.width << " h=" << roi.height << "] due to overlap" << std::endl;
				failed_consecutive_attempts ++;
				continue;
			}

			// ...otherise, if we get here then we seem to be covering a new part of the image
			failed_consecutive_attempts = 0;
			all_previous_rectangles.push_back(roi);
			zoom_txt
				<< original_image
				<< " -> creating RoI from [x=" << roi.x << " y=" << roi.y << " w=" << roi.width << " h=" << roi.height << "]" << std::endl;

			// Crop the original image, and at the same time resize it to be the exact dimensions we need.
			cv::Mat output_mat;
			cv::resize(original_mat(roi), output_mat, desired_size);

			std::stringstream ss;
			ss << dir_name << "/" << std::setfill('0') << std::setw(8) << all_output_images.size();
			const std::string output_base_name = ss.str();
			const std::string output_image = output_base_name + ".jpg";
			const std::string output_label = output_base_name + ".txt";

			cv::imwrite(output_image, output_mat, {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 75});
			all_output_images.push_back(output_image);

			// Read the annotations, crop them to match the image, and re-calculate the values for the .txt file.

			json root = json::parse(File(original_image).withFileExtension(".json").loadFileAsString().toStdString());
			std::ofstream fs_txt(output_label);
			fs_txt << std::fixed << std::setprecision(10);
			size_t number_of_annotations = 0;
			for (auto j : root["mark"])
			{
				int x = j["rect"]["int_x"];
				int y = j["rect"]["int_y"];
				int w = j["rect"]["int_w"];
				int h = j["rect"]["int_h"];
				const cv::Rect annotation_rect(x, y, w, h);
				const cv::Rect intersection = annotation_rect & roi;
				if (intersection.area() == 0)
				{
					// this annotation does not appear in our new image
					continue;
				}

				const int class_idx = j["class_idx"];

				if (x < roi.x)
				{
					// X is beyond the left border, we need to move it to the right
					int delta = roi.x - x;
					x += delta;
					w -= delta;
				}
				if (y < roi.y)
				{
					// Y is beyond the top border, we need to move it down
					int delta = roi.y - y;
					y += delta;
					h -= delta;
				}
				if (x + w > roi.x + roi.width)
				{
					// width is beyond the right border
					w = roi.x + roi.width - x;
				}
				if (y + h > roi.y + roi.height)
				{
					// height is beyond the bottom border
					h = roi.y + roi.height - y;
				}

				if (w < 5 or h < 5)
				{
					// ignore extremely tiny slices of annotations
					continue;
				}

				// bring all the coordinates back down to zero
				x -= roi.x;
				y -= roi.y;

				const double normalized_w = static_cast<double>(w) / static_cast<double>(roi.width	);
				const double normalized_h = static_cast<double>(h) / static_cast<double>(roi.height	);
				const double normalized_x = static_cast<double>(x) / static_cast<double>(roi.width	) + normalized_w / 2.0;
				const double normalized_y = static_cast<double>(y) / static_cast<double>(roi.height	) + normalized_h / 2.0;

				fs_txt << class_idx << " " << normalized_x <<  " " << normalized_y << " " << normalized_w << " " << normalized_h << std::endl;
				number_of_annotations ++;
			}

			number_of_marks += number_of_annotations;
			number_of_zooms_created ++;

			zoom_txt
				<< original_image
				<< " [" << original_mat.cols << "x" << original_mat.rows << "]"
				<< " -> " << output_image
				<< " [f=" << factor
				<< " x=" << roi.x
				<< " y=" << roi.y
				<< " w=" << roi.width
				<< " h=" << roi.height
				<< "]"
				<< " -> [" << output_mat.cols << "x" << output_mat.rows << "]"
				<< " [" << number_of_annotations << "/" << root["mark"].size() << "]"
				<< std::endl;
		}
	}

	return;
}
