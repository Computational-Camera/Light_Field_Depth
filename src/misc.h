//Miscellaneous Functions implementations basically for testing and dispalying the disparity results.

#ifndef _MISC
#define _MISC

#include <limits>
#include <opencv2/opencv.hpp>
#include "WMF/JointWMF.h"
#include "light_field.h"


using namespace std;
using namespace cv;

/**
    Convert integer disparity value to depth in float type.
    @depth          input disparity (uchar)
    @depth2         output depth (float)
    @lf_ptr         light field strutue pointer
*/
void label2depth( Mat& depth, Mat& depth2, LF* lf_ptr){

    for (int j = 0; j < lf_ptr->H; j++)
        for (int i = 0; i < lf_ptr->W; i++) 
            depth2.at<float>(j,i)=lf_ptr->dt_min+(float)depth.at<uchar>(j,i)*float(lf_ptr->dt_max-lf_ptr->dt_min)/(float)lf_ptr->nlabels;        
}

/**
    Convert integer disparity value to depth in float type.
    @depth          input as disparity (uchar), output as depth (float)
    @lf_ptr         light field strutue pointer
*/
void label2depth2( Mat& depth,LF* lf_ptr){

    for (int j = 0; j < lf_ptr->H; j++)
        for (int i = 0; i < lf_ptr->W; i++) 
            depth.at<float>(j,i)=lf_ptr->dt_min+depth.at<float>(j,i)*float(lf_ptr->dt_max-lf_ptr->dt_min)/(float)lf_ptr->nlabels;     
}


/**
    Color coding the disparity map based on confidence. 
    @img         the input should be single channel and float type
    @dir         file name to save
    @flag        dispaly background or not
*/
void color_map_confidence(LF* lf_ptr, const Mat& img, const char* dir, float c, float* cx, float* cy, int flag){
	Mat dst;
	Mat img2=img.clone();
	float min_tmp=1000000;
	//find the min
    for (int j=0;j<img.rows;j++){
     	for (int i=0;i<img.cols;i++){	        
     	        if (min_tmp>img.at<float>(j,i))
     	            min_tmp = img.at<float>(j,i);
    	    }     	    
     }
    //cout<<min_tmp<<endl;
    //fill unreliable pixel to minimium (background)
    for (int j=0;j<img.rows;j++){
     	for (int i=0;i<img.cols;i++){
     	    if (isnan(img.at<float>(j,i))){    	        
                img2.at<float>(j,i)=min_tmp-0.1;
    	    }
    	    //if (img2.at<float>(j,i)>1)
    	    //  img2.at<float>(j,i) =1 ;  
    	    //else if (img2.at<float>(j,i)<-1)
    	    //  img2.at<float>(j,i) =-1 ;  	    
     	}
     }
     
    normalize(img2, dst, 0, 255, NORM_MINMAX, CV_8UC1); 
	Mat cm_img0;
	
    // Apply the colormap:
    applyColorMap(dst, cm_img0, COLORMAP_JET);
	int cnt=0;
	if (flag>0){
	//make unreliable pixel to dark

        for (int j=0;j<cm_img0.rows;j++){
         	for (int i=0;i<cm_img0.cols;i++){
         	    //if (  (cy[j*cm_img0.cols+i]<c) ){
                if (  (cx[j*cm_img0.cols+i]<c)&&(cy[j*cm_img0.cols+i]<c) ){
                    cnt++;
                    cm_img0.at<Vec3b>(j,i)[0] = 0;	                       
                    cm_img0.at<Vec3b>(j,i)[1] = 0;	
                    cm_img0.at<Vec3b>(j,i)[2] = 0;	   
                }                     		    
		    }
	    }
	    
	}
	cout<<" "<<100*(1-double(cnt)/double(cm_img0.rows*cm_img0.cols))<<"%"<<endl;
    imwrite(dir,cm_img0);
}

/**
    Color coding the disparity map. 
    @img         the input should be single channel and float type
    @dir         file name to save
    @flag        dispaly background or not
*/
void color_map(const Mat& img, const char* dir, int flag){
	Mat dst;
	Mat img2=img.clone();
	float min_tmp=1000000;
	//find the min
    for (int j=0;j<img.rows;j++){
     	for (int i=0;i<img.cols;i++){	        
     	        if (min_tmp>img.at<float>(j,i))
     	            min_tmp = img.at<float>(j,i);
    	    }     	    
     }
    //cout<<min_tmp<<endl;
    //fill unreliable pixel to minimium (background)
    for (int j=0;j<img.rows;j++){
     	for (int i=0;i<img.cols;i++){
     	    if (isnan(img.at<float>(j,i))){    	        
                img2.at<float>(j,i)=min_tmp-0.1;
    	    }     	    
     	}
     }
     
    normalize(img2, dst, 0, 255, NORM_MINMAX, CV_8UC1); 
	Mat cm_img0;

    // Apply the colormap:
    applyColorMap(dst, cm_img0, COLORMAP_JET);
	
	if (flag==1){
	//make unreliable pixel to dark
        for (int j=0;j<cm_img0.rows;j++){
         	for (int i=0;i<cm_img0.cols;i++){
                if (   (cm_img0.at<Vec3b>(j,i)[0] == 128)
                    && (cm_img0.at<Vec3b>(j,i)[1] ==   0)
                    && (cm_img0.at<Vec3b>(j,i)[2] ==   0)
                   )
                    cm_img0.at<Vec3b>(j,i)[0] = 0;	                                            		    
		    }
	    }
	}
	
    imwrite(dir,cm_img0);
}

/**
    grey coding the disparity map. 
    @img         the input should be single channel and float type
    @dir         file name to save
    @flag        dispaly background or not
*/
void grey_map(const Mat& img, const char* dir, int flag){
	Mat dst;
	Mat img2=img.clone();
    normalize(img2, dst, 0, 255, NORM_MINMAX, CV_8UC1);
    imwrite(dir,dst); 
	imwrite(dir,img2);

}

/**
    Merge two disparity maps to one based on the ground truth.
    This is used for evaluate our merge function.
    @data_best_x        input 1
    @data_best_y        input 2
    @data_best_xy       output result
    @lf_ptr             light field strutue pointer
*/
void depth_merge_gt(  uchar* data_best_x, uchar* data_best_y, uchar* data_best_xy, LF* lf_ptr){

    int height = lf_ptr->H;
    int width  = lf_ptr->W;

    Mat depth_matx( height, width, CV_8U, data_best_x);
    Mat depth_maty( height, width, CV_8U, data_best_y);  
    Mat depth_matx2( height, width, CV_32F);
    Mat depth_maty2( height, width, CV_32F);        
    label2depth( depth_matx, depth_matx2, lf_ptr);      
    label2depth( depth_maty, depth_maty2, lf_ptr);

    for (int j=0; j<lf_ptr->H; j++)
	    for (int i=0; i<lf_ptr->W; i++){

            if (lf_ptr->disparity_mask.at<float>(j,i)>0){

                float pix_errx = fabs( depth_matx2.at<float>(j,i) - lf_ptr->disparity_gt.at<float>(j,i) );	
                float pix_erry = fabs( depth_maty2.at<float>(j,i) - lf_ptr->disparity_gt.at<float>(j,i) );
            
                if (pix_errx<pix_erry)
                    data_best_xy[j*lf_ptr->W+i]=data_best_x[j*lf_ptr->W+i];
                else 
                    data_best_xy[j*lf_ptr->W+i]=data_best_y[j*lf_ptr->W+i];
            }
        }
}

/**
    Results evaluation function.
    @depth           The disparity result as input
    @depth_gt        The ground truth as input
    @depth_gt_mask   The make used for removing the background pixels.
    @dir             file name to save. A color coded error map. 
*/
void error_comparison( Mat& depth, const Mat& depth_gt, const Mat& depth_gt_mask, const char* dir){

  size_t errors_1pix  = 0;
  size_t errors_05pix = 0;
  size_t errors_01pix = 0;
  size_t errors_005pix = 0;
  double avg_abs_err   = 0.0;
  double avg_sq_err    = 0.0;

  int    count=0;
  Mat gt=depth_gt.clone();
  Mat dd=depth.clone();  
  Mat error_map = Mat::zeros(depth.size(), CV_32F);

for (int j=6; j<(depth.rows-6); j++)
	for (int i=6; i<(depth.cols-6); i++){

		if (depth_gt_mask.at<float>(j,i)>0){
		    		   
            if (isnan(depth.at<float>(j,i))) cout<<j<<" "<<i<<endl;
			float pix_err = fabs( depth.at<float>(j,i) - depth_gt.at<float>(j,i) );
            error_map.at<float>(j,i)=pix_err<0.05 ? 0 : pix_err;
            error_map.at<float>(j,i)=pix_err>0.1 ? 0.1 : pix_err;                 
			if ( pix_err > 1.0 ) {
			  errors_1pix ++;
			}
			if ( pix_err > 0.5 ) {
			  errors_05pix ++;
			}
			if ( pix_err > 0.1 ) {
			  errors_01pix ++;
			}
			if ( pix_err > 0.05 ) {
			  errors_005pix ++;
			}

			avg_abs_err += pix_err;
			avg_sq_err += pow( pix_err, 2.0 );
		    count++;
		}

	}
  avg_abs_err /= double(count);
  avg_sq_err /= double(count);
  //mat2hdf5 ("depth.h5", "data", H5T_NATIVE_FLOAT, float(), dd);  
  //mat2hdf5 ("error.h5", "data", H5T_NATIVE_FLOAT, float(), error_map);
  //mat2hdf5 ("gt.h5",    "data", H5T_NATIVE_FLOAT, float(), gt);
  //color_map(error_map,dir,0);
  //color_map(depth_gt, "./debug/data/depth_xy_gt.png", 0);
  cout<< "Error >   1 disp  [ in % of pixels ]: " << double(errors_1pix)/double(count)  * 100.0 << endl ;
  cout<< "Error > 0.5 disp  [ in % of pixels ]: " << double(errors_05pix)/double(count) * 100.0 << endl ;
  cout<< "Error > 0.1 disp  [ in % of pixels ]: " << double(errors_01pix)/double(count) * 100.0 << endl ;
  cout<< "Error > 0.05 disp [ in % of pixels ]: " << double(errors_005pix)/double(count)* 100.0 << endl ;
  cout<< endl;
  cout<< "Average absolute error         : " << avg_abs_err << endl;
  cout<< "Average squared error          : " << avg_sq_err << endl;
  cout<<(float)100*count/(float)(depth.rows*depth.cols)<<"% pixel are checked"<<endl;
  cout<< endl;
}

/**
    Results evaluation wrapper including pre-filtering.
    @depth           The disparity result as input
    @filename        file name to save. A color coded error map
    @lf_ptr          light field strutue pointer 
*/

void evaluate_depth(uchar* depth, const char* filename, LF* lf_ptr){

    int width  = lf_ptr->W;
    int height = lf_ptr->H;

    Mat depth_mat2( height, width, CV_8U, depth);
    
    //===median blur
    //medianBlur ( depth_mat2, depth_mat2, 5);
    
    //===content aware blur
    Mat img_grey;
    cvtColor(lf_ptr->imgc, img_grey, CV_RGB2GRAY);
    JointWMF wmf;
    depth_mat2=wmf.filter(depth_mat2, img_grey, 5, 25.5, 64, 256, 1, "exp");//cos
    Mat depth_mat( height, width, CV_32F);
    label2depth( depth_mat2, depth_mat, lf_ptr);
    error_comparison(depth_mat, lf_ptr->disparity_gt, lf_ptr->disparity_mask, filename);  
}
#endif

