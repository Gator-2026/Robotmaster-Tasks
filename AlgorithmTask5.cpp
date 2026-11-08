#include<opencv2/opencv.hpp>
#include<opencv2/geometry.hpp>
#include<iostream>
#include<vector>
#include<string>
#include<cmath>

using namespace std;
static vector<vector<cv::Point2f>> vitalpointslist;

struct rect_and_avail {
	cv::RotatedRect rore;
	bool if_avail;
};

float distance(cv::Point2f p1, cv::Point2f p2) {
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

cv::Point2f midpoint(cv::Point2f p1, cv::Point2f p2){
	return cv::Point2f((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
}

float longeredge(cv::Size size) {
	return max(size.width, size.height);
}

bool if_light_ok(vector<cv::Point> contour){
	bool if_ok = true;
	float whratio = cv::minAreaRect(contour).size.width / cv::minAreaRect(contour).size.height;
	if (whratio > 1) {
		whratio = 1.0 / whratio;
	}
	if (whratio > 0.3334)if_ok = false;
	cv::RotatedRect rore = cv::minAreaRect(contour);
	if (rore.size.area() < 50)if_ok = false;
	if (abs(rore.angle) < 66)if_ok = false;
	return if_ok;
}

bool if_light_match(cv::RotatedRect r1, cv::RotatedRect r2) {
	bool if_ok = true;
	/*cout << "angle's difference" << abs(r1.angle - r2.angle) << endl;
	cout << "size match:" << abs(longeredge(r1.size) - longeredge(r2.size)) / longeredge(r2.size) << endl;
	cout << "if y of centers match:" << abs(r1.center.y - r2.center.y) / longeredge(r2.size) << endl;
	cout << "if ratio of board reasonable:" << abs(distance(r1.center, r2.center)) / longeredge(r1.size) - 2 << endl;*/
	if (abs(r1.angle - r2.angle) > 0.1)if_ok = false;
	if (abs(longeredge(r1.size) - longeredge(r2.size))/longeredge(r2.size) > 0.1)if_ok = false;
	if (abs(r1.center.y - r2.center.y)/longeredge(r2.size) > 0.5)if_ok = false;
	if (abs(distance(r1.center, r2.center) / longeredge(r1.size) - 2) > 0.75)if_ok = false;
	cout << endl;
	return if_ok;
}

cv::Mat getlightmask(cv::Mat&img) {
	if (img.empty()) {
		std::cout<< "Failed to read the image" << endl;
		return img;
	}
	cv::Mat imgcopy = img.clone();
	cv::Mat hsv;
	cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);
	cv::Mat mask, mask2, mask3;
	cv::inRange(hsv, cv::Scalar(4, 110, 150), cv::Scalar(52, 255, 255), mask);
	cv::medianBlur(mask,mask2,3);
	cv::morphologyEx(mask2, mask3, cv::MORPH_CLOSE,cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
	cv::Mat result,result1;
	img.copyTo(result, mask3);
	imgcopy.copyTo(result, mask3);
	cv::imshow("mask3", mask3);
	cv::waitKey(0);
	vector<vector<cv::Point>> contours;
	vector<cv::Vec4i> hierarchy;
	cv::findContours(mask3, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	int cs = contours.size();
	vector<rect_and_avail> rects(cs);
	for (int i = 0;i < cs;i++) {
		/*cv::Point2f a[4];
		cv::minAreaRect(contours[i]).points(a);
		for (int j = 0;j < 4;j++) {
			cv::line(img, a[j % 4], a[(j + 1) % 4], cv::Scalar(0, 0, 255), 1, cv::LINE_8);
		}
		cv::putText(img,to_string(i), cv::minAreaRect(contours[i]).center, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 0, 0));
		cout << i << endl;
		cout << "area:"<<minAreaRect(contours[i]).size.area() << endl;
		cout << "angle:"<<minAreaRect(contours[i]).angle << endl;*/
		if (if_light_ok(contours[i])) {
			rects[i].rore = cv::minAreaRect(contours[i]);
			rects[i].if_avail = true;
			cv::Point2f a[4];
			cv::minAreaRect(contours[i]).points(a);
			for (int j = 0;j < 4;j++) {
			cv::line(imgcopy, a[j % 4], a[(j + 1) % 4], cv::Scalar(0, 0, 255), 1, cv::LINE_8);
			}
			cv::putText(imgcopy,to_string(i), cv::minAreaRect(contours[i]).center, cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 0, 0));
		}
		else {
			rects[i].if_avail = false;
		}
	}
	cv::imshow("possiblelights", imgcopy);
	cv::imwrite("output/possiblelights.png", imgcopy, { cv::IMWRITE_PNG_COMPRESSION,3 });
	//cv::Mat blackmask, blackmask1;
	//cv::inRange(img, cv::Scalar(0, 16, 10), cv::Scalar(255, 32, 36), blackmask);
	//cv::imshow("blackmask", blackmask);
	//cv::waitKey(0);
	//cv::imwrite("testphoto2/blackmask-0-16-10-255-32-36.png", blackmask, { cv::IMWRITE_PNG_COMPRESSION,3 });//
	for (int i = 0;i < cs;i++) {
		if (!rects[i].if_avail)continue;
		for (int j = i+1;j < cs;j++) {
			if (!rects[j].if_avail)continue;
			//cout << i << " " << j ;
			if (if_light_match(rects[i].rore,rects[j].rore)) {
				cv::RotatedRect r1;
				cv::RotatedRect r2;
				if (rects[i].rore.center.x < rects[j].rore.center.x) {
					r1 = rects[i].rore;
					r2 = rects[j].rore;
				}
				else {
					r1 = rects[j].rore;
					r2 = rects[i].rore;
				}
				//cout << r1.center << ' ' << r2.center << endl;
				vector<cv::Point2f>repoints(4);
				int le = longeredge(rects[i].rore.size);
				cv::Point2f centerpoint = midpoint(rects[i].rore.center, rects[j].rore.center);
				int widthofboard = distance(rects[i].rore.center, rects[j].rore.center);
				int plusandminor[2] = { -1,1 };
				int q = -1;
				int p = 0;
				int r = 0;
				vector<cv::Point2f> vitalpoints(5);
				vitalpoints[0] = centerpoint;
				cout<< "centerpoint is : "<<centerpoint << endl;
				cout << "Four cornors:" << endl;
				for (int& k : plusandminor) {
					q += (-k);
					for (int& l : plusandminor) {
						r++;
						cv::Point2f ctp = centerpoint;
						ctp.x += plusandminor[q] * widthofboard / 2;
						ctp.y += (k+0.25*(abs(k)+ k)/2) * le;
						repoints[p++] = ctp;
						cout << ctp << " ";
						q += (- k);
						vitalpoints[r] = ctp;
					}
				}
				vitalpointslist.push_back(vitalpoints);
				cout << endl;
				for (int k = 0;k < 4;k++) {
					cv::line(img, repoints[k % 4], repoints[(k + 1) % 4], cv::Scalar(0, 0, 255), 1, cv::LINE_8);
				}
				cout << "The result is effective." << endl;
				/*cv::Point2f anglepoints1[4], anglepoints2[4];
				r1.points(anglepoints1);
				r2.points(anglepoints2);
				cv::Point2f ap1min = anglepoints1[0];
				cv::Point2f ap2max = anglepoints2[0];
				for (auto& i : anglepoints1) {
					if (i.y < ap1min.y) {
						ap1min = i;
					}
				}
				for (auto& i : anglepoints2) {
					if (i.y > ap2max.y) {
						ap2max = i;
					}
				}
				for (int k = 0;k < 4;k++) {
					cv::rectangle(img, ap1min, ap2max, cv::Scalar(0, 255, 0), 2, 8);
				}*/
			}
		}
	}
	cv::imshow("detect", img);
	cv::imwrite("output/detect.png", img, { cv::IMWRITE_PNG_COMPRESSION,3 });
	cv::waitKey(0);
	return mask2;
}

int main() {
	string path;
	getline(cin, path);
	cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
	cv::Mat img2 = getlightmask(img);
	cv::imshow("lightmask", img2);
	cv::imwrite("output/lightmask.png", img2);
	for (auto& i : vitalpointslist) {
		cout << i << " ";
	}
	return 0;
}