#ifndef _POLIGONDRAW_H_INCLUDED_
#define _POLIGONDRAW_H_INCLUDED_

class CNetImage;

class CPoligonDraw {
public:
	static void DrawPoligon(CNetImage *_image, int *_points,
			unsigned int _pointcount);
};

#endif 
