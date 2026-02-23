#include "PoligonDraw.h"
#include "NetImage.h"

#include <vector>

using namespace std;

static float LinTran(float _pos1, float _begin1, float _end1, float _begin2,
		float _end2) {

	return _begin2 + (_pos1 - _begin1) / (_end1 - _begin1) * (_end2 - _begin2);
}

void CPoligonDraw::DrawPoligon(CNetImage *_image, int *_points,
		unsigned int _pointcount) {

	_image->FillImage(0);

	if (_pointcount == 0) {
		return;
	}

	int miny = _points[1];
	int maxy = miny;
	for (unsigned int p_i = 1; p_i < _pointcount; p_i++) {
		int cury = _points[p_i * 2 + 1];
		if (miny > cury) {
			miny = cury;
		}
		if (maxy < cury) {
			maxy = cury;
		}
	}
	if ((maxy < 0) || (miny >= (int) _image->height_)) {
		return;
	}

	vector<int> crossings;

	for (int y = 0; y < (int) _image->height_; y++) {

		crossings.clear();
		for (unsigned int p_i = 0; p_i < _pointcount; p_i++) {
			int x1 = _points[p_i * 2];
			int x2 = _points[((p_i + 1) % _pointcount) * 2];
			float y1 = (float) _points[p_i * 2 + 1];
			float y2 = (float) _points[((p_i + 1) % _pointcount) * 2 + 1];

			if ((y1 == y2) && (y == y1)) {

				unsigned char *lineptr = _image->GetPtr(0, y);
				for (int xd = x1; xd <= x2; xd++) {
					lineptr[xd] = 255;
				}
			} else {
				y1 += 0.1f;
				y2 += 0.1f;
				if (((y1 > y) && (y2 < y)) || ((y1 < y) && (y2 > y))) {

					int x = (int) (LinTran((float) y, (float) y1, (float) y2,
							(float) x1, (float) x2) + 0.5f);
					crossings.push_back(x);
				}
			}
		}

		if (!crossings.empty()) {

			for (unsigned int x_i1 = 0; x_i1 < crossings.size() - 1; x_i1++) {
				for (unsigned int x_i2 = x_i1 + 1; x_i2 < crossings.size();
						x_i2++) {
					if (crossings[x_i1] > crossings[x_i2]) {
						int s = crossings[x_i1];
						crossings[x_i1] = crossings[x_i2];
						crossings[x_i2] = s;
					}
				}
			}

			unsigned char *lineptr = _image->GetPtr(0, y);
			for (unsigned int c_i = 0; c_i < crossings.size(); c_i += 2) {
				int x1 = crossings[c_i];
				int x2 = crossings[c_i + 1];
				if ((x2 > 0) && (x1 < (int) _image->width_)) {
					if (x1 < 0) {
						x1 = 0;
					}
					if (x2 >= (int) _image->width_) {
						x2 = _image->width_ - 1;
					}
					for (int xd = x1; xd <= x2; xd++) {
						lineptr[xd] = 255;
					}
				}
			}
		}
	}
}
