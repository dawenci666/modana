#ifndef VIDEO_GENERATOR_H
#define VIDEO_GENERATOR_H

/**
 * @brief Generate a video from images in a directory/images folder.
 * 
 * The function looks for an "images" subdirectory inside `directoryname`.
 * If it exists, collects all image files (.png, .jpg, .jpeg), sorts them,
 * and creates a video using ffmpeg.
 * 
 * @param directoryname Path to the base directory containing "images" folder.
 * @param video_filename Output video filename. If NULL, defaults to "animation.mp4" inside directoryname.
 * @param fps Frames per second for the video. If <= 0, defaults to 3.
 */
void generate_video_from_images(const char *directoryname, const char *video_filename, int fps);

#endif /* VIDEO_GENERATOR_H */
