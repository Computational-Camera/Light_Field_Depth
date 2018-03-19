//  Define the data structure of light field plus multiview to light field slice conversion implementation.

#ifndef _LF
#define _LF

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;
/**
    the structure of light field containing all parameters and data
*/
typedef struct {

    //parameters
    int W;
    int H;
    int U;
    int V;
    int nlabels;
    int type; // 0: HCI, 1: LYTRO
    int mask;
    float conf;
    float d_min;
    float d_max;
    float dt_min;
    float dt_max;
    int   threshold;//for mrf
    float lambda;  //for mrf
    double focalLength;
    double shift;
    double baseline;

    //file_name
    string disparity_filename;
    string data_filename;
    string depth_filename;
    string depth_filter_filename;
    string erro_map_filename;
    string centre_view_filename;

    //data container
    vector<Mat> epi_h, epi_v;
    unsigned char* lf_raw;
    Mat depth;
    Mat depth_f;
    Mat depth_x;
    Mat depth_y;
    Mat confidence;
    Mat confidence_x;
    Mat confidence_y;  
    Mat disparity_gt;
    Mat disparity_mask;
    Mat img, imgc;   

}LF;


/**
    Extract the vertical and horizontal epi slices from multiviw image array (colour).
    @epi_h   the horizontal slice vector
    @epi_v   the vertical slice vector     
    @lf_ptr  the pointer of light field structure    
*/
void mview2epis(vector<Mat>& epi_h, vector<Mat>& epi_v, LF* lf_ptr){

	epi_h.resize(lf_ptr->W);
	epi_v.resize(lf_ptr->W);	

    for (int i=0; i<lf_ptr->W; i++){
        epi_h[i] = Mat(lf_ptr->V, lf_ptr->W, CV_32FC3);
        epi_v[i] = Mat(lf_ptr->U, lf_ptr->H, CV_32FC3);  
    }
    
	//mulitple view denoising
	/*
    for (int m = 1; m < lf_ptr->V-1; m++)
		for (int n = 1; n < lf_ptr->U-1; n++){
			fastNlMeansDenoisingColored(lf_ptr->img(Rect(n*lf_ptr->W,m*lf_ptr->H,lf_ptr->W,lf_ptr->H)), 
                                        lf_ptr->img(Rect(n*lf_ptr->W,m*lf_ptr->H,lf_ptr->W,lf_ptr->H)), 3, 3, 1, 9);
	}
	imwrite("tuilip.png", lf_ptr->img);
	*/
    //extract central view
    lf_ptr->imgc = Mat(lf_ptr->H,lf_ptr->W,CV_8UC3);

    for (int j=0; j<lf_ptr->H; j++) 
        for (int i = 0; i < lf_ptr->W; i++)
            lf_ptr->imgc.at<Vec3b>(j,i)=lf_ptr->img.at<Vec3b>(lf_ptr->H*(lf_ptr->V-1)/2+j,lf_ptr->W*(lf_ptr->U-1)/2+i); 

	
    imwrite(lf_ptr->centre_view_filename.c_str(), lf_ptr->imgc);

    //===horizontal epi slices===
    for (int j=0; j<lf_ptr->H; j++){
        for (int n = 0; n < lf_ptr->U; n++)
            for (int i = 0; i < lf_ptr->W; i++){ 			
				Vec3f a = lf_ptr->img.at<Vec3b>(lf_ptr->H*((lf_ptr->V-1)/2)+j,n*lf_ptr->W+i);
				//Vec3f b = lf_ptr->img.at<Vec3b>(lf_ptr->H*((lf_ptr->V-1)/2-1)+j,n*lf_ptr->W+i);	
				//Vec3f c = lf_ptr->img.at<Vec3b>(lf_ptr->H*((lf_ptr->V-1)/2+1)+j,n*lf_ptr->W+i);
				//Vec3f d = lf_ptr->img.at<Vec3b>(lf_ptr->H*((lf_ptr->V-1)/2-2)+j,n*lf_ptr->W+i);	
				//Vec3f e = lf_ptr->img.at<Vec3b>(lf_ptr->H*((lf_ptr->V-1)/2+2)+j,n*lf_ptr->W+i);
                epi_h[j].at<Vec3f>(n,i)  = a;//(a+b+c+d+e)*0.2;   
            }    
         //if (j==100) imwrite("./debug/epi_v100.png", epi_h[j]);    
         //if (j==200) imwrite("./debug/epi_v100.png", epi_h[j]);          
         //if (j==400) imwrite("./debug/epi_v400.png", epi_h[j]);      
    }        
   
    //===vertical epi slices===
    for (int i=0; i<lf_ptr->W; i++) 
        for (int m = 0; m < lf_ptr->V; m++)
            for (int j = 0; j < lf_ptr->H; j++) { 
            
                if (lf_ptr->type==0){//HCI{
                epi_v[i].at<Vec3f>(lf_ptr->V-1-m,j)
                 = (Vec3f) lf_ptr->img.at<Vec3b>(m*lf_ptr->H+j, lf_ptr->W*((lf_ptr->U-1)/2)+i);                 
                }  
                else  if (lf_ptr->type==1){//LYTRO}
				Vec3f a = lf_ptr->img.at<Vec3b>(m*lf_ptr->H+j, lf_ptr->W*( lf_ptr->U-1)/2   +i);
				//Vec3f b = lf_ptr->img.at<Vec3b>(m*lf_ptr->H+j, lf_ptr->W*((lf_ptr->U-1)/2-1)+i);
				//Vec3f c = lf_ptr->img.at<Vec3b>(m*lf_ptr->H+j, lf_ptr->W*((lf_ptr->U-1)/2+1)+i);
				//Vec3f d = lf_ptr->img.at<Vec3b>(m*lf_ptr->H+j, lf_ptr->W*((lf_ptr->U-1)/2-1)+i);
				//Vec3f e = lf_ptr->img.at<Vec3b>(m*lf_ptr->H+j, lf_ptr->W*((lf_ptr->U-1)/2+1)+i);
                epi_v[i].at<Vec3f>(m,j) = a;//(a+b+c+d+e)*0.2;                   
                //if (i==500) imwrite("./debug/epi_v500.png", epi_v[i]);    
				}                      
            }                       
}

#endif
