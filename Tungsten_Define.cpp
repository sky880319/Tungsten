#include "Tungsten_Define.h"
#include <cmath>

TgPoint parseVisionCoordinate(const cv::Point& img_point, const cv::Mat& img)
{
	int x, y;
	x = img.rows / 2 - img_point.y;
	y = img.cols / 2 - img_point.x;

	return TgPoint(x, y, 0);
}

TgWorld parseWorldCoordinate(TgPoint& vision_position)
{
	float x, y, z;
	x = HOME_X + VISION_COOR_X - vision_position.X() * cosf(VISION_COOR_RAD) * PEXEL2MM_FACTOR;
	y = HOME_Y + VISION_COOR_Y - vision_position.Y() * cosf(VISION_COOR_RAD) * PEXEL2MM_FACTOR;
	z = vision_position.Z();
	return TgWorld(x, y ,z, 0.f);
}