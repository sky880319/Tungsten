#include "Tungsten_Define.h"
#include <cmath>

TgPoint parseVisionCoordinate(const cv::Point& img_point, const cv::Mat& img, float meter_depth)
{
	int x, y, z;
	x = img.rows / 2 - img_point.y;
	y = img.cols / 2 - img_point.x;
	z = meter_depth * 1000;

	return TgPoint(x, y, z);
}

TgWorld parseWorldCoordinate(TgPoint& vision_position)
{
	float x, y, z;
	x = HOME_X + VISION_COOR_X - vision_position.X() * cosf(VISION_COOR_RAD) * PEXEL2MM_FACTOR_X;
	y = HOME_Y + VISION_COOR_Y - vision_position.Y() * cosf(VISION_COOR_RAD) * PEXEL2MM_FACTOR_Y;
	z = HOME_Z + VISION_COOR_Z - vision_position.Z();
	return TgWorld(x, y ,z, z);
}