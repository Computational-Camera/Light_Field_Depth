// Extact depth from light field.

#ifndef _DEPTH_FROM_LIGHTFIELD
#define _DEPTH_FROM_LIGHTFIELD
#include <limits>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "WMF/JointWMF.h"
#include "gco/GCoptimization.h"
#include "light_field.h"
#include "lf2depth_mrf.h"
#include "volume_filtering.h"
#include "misc.h"

#define DEBUG

using namespace std;
using namespace cv;
float* d;

/**
    final result filtering using fast Weighted Median Filter(WMF)
    @lf_ptr          light field strutue pointer 
*/
//======output the depth mapped filtered with the central view
void depth_filtering(LF* lf_ptr){
    Mat img_grey;
    cvtColor(lf_ptr->imgc, img_grey, CV_RGB2GRAY);
    JointWMF wmf;
    lf_ptr->depth_f=wmf.filter(lf_ptr->depth, img_grey, 5, 25.5, 64, 256, 1, "exp");//cos
    //medianBlur ( lf.depth, lf.depth_f, 5 );
}

/**
    Calculate the disparity cost per pixel
    @img          EPI slice as input
    @depth        depth cost as output
    @depthc       depth confidence as output
    @lf_ptr       light field structure pointer 
*/

bool disparity_cost( const Mat& img,  float *depth, float *depthc,  LF* lf_ptr){
	
	Vec3f *data_ptr = (Vec3f*)(img.data); 	 
	int cc = (lf_ptr->U-1)/2;
	int nlabels = lf_ptr->nlabels;
	float* depth_addr2  = new float[img.cols*lf_ptr->nlabels];//depth  + 3*lf_ptr->nlabels;
	float* depth_addr3  = depth_addr2;
	depth_addr2  =  depth_addr2  + 3*nlabels;
	float* depthc_addr = depthc  + 3*nlabels;



	for (int i=3; i<(img.cols-3); i++){
     
		float data_new[3], err[3];	    
 
		for (int k=0; k<nlabels; k++){ //-64 64
  			
			float tmp1[3] = {0,0,0}, tmp2[3] = {0,0,0};	

		    for (int t=-3; t<=3; t++){

				float xnew  = (i + d[k] * float(t));//(i + d * float(t))*8;				      	 
		        int idx     = int(cc+t)*img.cols+int(xnew);
			
				float b = xnew- int(xnew);//bilinear interpolation
				float a = 1 - b;		

				for (int m=0; m<3; m++){
					data_new[m] = data_ptr[idx][m]*a + data_ptr[idx+1][m]*b ;  
					tmp1[m] = tmp1[m] + data_new[m];
					tmp2[m] = tmp2[m] + data_new[m]*data_new[m];	           		                            		 
				}
			}
 
			for (int m=0; m<3; m++)
				err[m] = tmp2[m] - tmp1[m]*tmp1[m]/7;
					  	  
			float err_max    = (err[0]  > err[1])? err[0]  : err[1];
			*depth_addr2++   = (err_max > err[2])? err_max : err[2];
	 		*depthc_addr++   =  (tmp1[0]+tmp1[1]+tmp1[2])/21;
		}   		 			 
	}


	float* depth_addr  = depth  + 4*nlabels;

	for (int i=4; i<(img.cols-4); i++)
		for (int k=0; k<nlabels; k++)
			*depth_addr++   = depth_addr3[(i-1)*nlabels +k] + depth_addr3[i*nlabels +k] + depth_addr3[(i+1)*nlabels+k];


    return true;
}

/**
    Build the cost volume.
    @epi_h         Horizontal EPI slices as input
    @epi_v         Vertical   EPI slices as input
    @depth_x       Horizontal cost volume
    @depth_y       Vertical   cost volume
    @depth_cx      Horizontal confidence volume
    @depth_cy      Vertical   confidence volume
    @lf_ptr        light field structure pointer         
*/
void cost_volume(     vector<Mat>& epi_h, 
                	  vector<Mat>& epi_v,  
                      float* depth_x,
			          float* depth_y,
                      float* depth_cx,
			          float* depth_cy,			          
                      LF* lf_ptr){
    
    //discrete depth value
    d  = new float[lf_ptr->nlabels+1];   
    
    float dmin=lf_ptr->d_min;
    float dmax=lf_ptr->d_max;   
  	for (int k=0; k<=lf_ptr->nlabels; k++){
		d[k]= dmin+float(k)*(dmax-dmin)/float(lf_ptr->nlabels);
	    //cout<<d[k]<<endl;	
    }

    //=============Horizontal==================
    #pragma omp parallel for
	for (int j = 0; j < lf_ptr->H; j++){ 
		int offset = lf_ptr->nlabels*j*lf_ptr->W;
		disparity_cost( epi_h[j], depth_x+offset, depth_cx+lf_ptr->nlabels*j*lf_ptr->W, lf_ptr);
	}
	cout<<" Extracting Horizontal EPI Slices Done"<<endl;

    //==============Vertical=====================
    #pragma omp parallel for
	for (int i = 0; i < lf_ptr->W; i++) {
		
        float *depth_array           =  (float*) calloc (lf_ptr->nlabels*lf_ptr->W, sizeof(float)); 
	    float *con_array             =  (float*) calloc (lf_ptr->nlabels*lf_ptr->W, sizeof(float)); 	         
            
        disparity_cost( epi_v[i], depth_array, con_array, lf_ptr);
			
		for (int j = 0; j < lf_ptr->H; j++){	
		int idx = j*lf_ptr->W+i;			    
	    memcpy(depth_y  + lf_ptr->nlabels*(idx), depth_array+ j*lf_ptr->nlabels, lf_ptr->nlabels * sizeof(float));
	    memcpy(depth_cy + lf_ptr->nlabels*(idx), con_array  + j*lf_ptr->nlabels, lf_ptr->nlabels * sizeof(float));		    
        }

        delete[] depth_array;
        delete[] con_array;
	}
	cout<<" Extracting Vertical   EPI Slices Done"<<endl;
    
}

/**
    Brute-force searching the optimal depth per pixel to produce the disparity map.
    @data          A cost volume as input
    @data_best     The disparity map as output
    @lf_ptr        light field structure pointer         
*/
bool depth_optimal( float* data, uchar* data_best, LF* lf_ptr){

	for (int i=0; i<lf_ptr->W*lf_ptr->H; i++){	
					
	    float value=10000000;
	    int offset = i*lf_ptr->nlabels;
        int idx =0;

	    for (int k=0; k<lf_ptr->nlabels; k++){
	        float d =  data[offset+k];
	        
	        //if (i==406*640+27) cout<<d<<endl;
	        if (value>d){
	           value=d;
	           idx = k;
	        }
	    }
	    data_best[i]=idx;	  
	    //cout<<idx<<endl;  
	}
    //cout<<"===================================="<<endl;
    return true;
}

/*
    Brute-force searching the optimal slope (depth) for a pixel.
    @data         A single stack for one pixel
    @num          The number of layer for the stack (volume)
    @idx          The optimal slope index
    @score        The score for the optimal slope           
*/
bool depth_optimal_pixel(float* data, int num, int& idx, float& score, float& ratio){

    idx = 0;
    float avg=0;
    float value_min=100000;
    float value_max=0;
    //float diff=0;
    
    for (int k=0; k<num; k++){

        avg = avg + data[k];

        if (value_min>data[k]){
            value_min=data[k];
            idx = k;
        }
        if (value_max<data[k]){
            value_max=data[k]; 
        }   
    }

    //find the total number o pixels that larger than average value
    int cnt = 0;
    avg = avg/num;
    for (int k=0; k<num; k++){
        if (data[k]>avg)
        cnt++;
    }
    ratio = (float)cnt/(float)num;
    score = avg/value_min;//float(cnt) / float(num);
    //cout<<avg<<" "<<value_min<<" "<<score<<endl;

return 0;//
}

/**
    Find the optimal disparity(depth) per pixel to produce the disaprity map
    based on the horizontal and vertical volume stacks.
    @data1   the horizontal volume (variance)
    @data2   the vertical   volume (variance)
    @conf1   the horizontal volume (average)
    @conf2   the vertical   volume (average)   
    @cf1     the horizontal image (average) 
    @cf2     the vertical   image (average)       
    @data_best  the 2D disaprity
    @lf_ptr  the light field structure pointer
*/
bool compute_slope_xy( float* data1, float* data2,
                 float* conf1, float* conf2, 
                 float* cf1,   float* cf2,  
                 uchar* data_best,                                                  
                 LF* lf_ptr){

    int height =  lf_ptr->H;
    int width  =  lf_ptr->W;
    int labels =  lf_ptr->nlabels;

    int cnt=0;
    float score[2], ratio[2];

    for (int j=0; j< height; j++){
	    for (int i=0; i< width; i++){
	    	
	    	if ((j>0)&&(i>0)&&(j<(height-1))&&(i<(width-1))){    					   

                int idx[2]    ={0,0};                   
                
                depth_optimal_pixel(&data1[cnt*labels], labels, idx[0], score[0], ratio[0]);
                depth_optimal_pixel(&data2[cnt*labels], labels, idx[1], score[1], ratio[1]); 
                
                int offsetx0  = cnt*labels +idx[0];
                int offsetx1 = (    j*width+i-1)*labels +idx[0];  

                int offsety0  = cnt*labels +idx[1];            
                int offsety1 = ((j-1)*width+i)*labels   +idx[1];


                float cx1 = fabs(conf1[offsetx0]-conf1[offsetx1]);
                float cy1 = fabs(conf2[offsety0]-conf2[offsety1]);           
				
				cf1[cnt]      = ((score[0]>2.5)&&(ratio[0]>0.4)) ? cx1 : 0;//=================
				cf2[cnt]      = ((score[1]>2.5)&&(ratio[1]>0.4)) ? cy1 : 0;//=================
				
				if (cf1[cnt]>0)
					data_best[j*width+i] = idx[0];
				if (cf2[cnt]>cf1[cnt])
					data_best[j*width+i] = idx[1];
            }
            cnt++;                
        }                     
	}
	return true;
}

void spatial_filtering(float* confidence_x, float* confidence_y,  LF* lf_ptr){

	int width  = lf_ptr->W;
    int height = lf_ptr->H;

	Mat binary_map   = Mat::zeros(height, width, CV_8U);
	Mat binary_map2  = Mat::zeros(height, width, CV_8U);
	Mat binary_map3  = Mat::zeros(height, width, CV_8U);	
	
	for (int y = 4; y < height-4; y++ )
		for (int  x = 4; x < width-4; x++ ){
		int idx = y*width+x;
 		if ((confidence_x[idx]>lf_ptr->threshold)||(confidence_y[idx]>lf_ptr->threshold))
				binary_map.at<uchar>(y,x) = 1;
	}
	//imwrite("test1.png", binary_map*255);

	for (int y = 4; y < height-4; y++ )
		for (int  x = 4; x < width-4; x++ ){

			if (binary_map.at<uchar>(y,x)){
				//int idx = y*width+x;
				int cnt1 = binary_map.at<uchar>(y-1,x)+   binary_map.at<uchar>(y+1,x);
				int cnt2 = binary_map.at<uchar>(y,x-1)+   binary_map.at<uchar>(y,x+1);
				int cnt3 = binary_map.at<uchar>(y-1,x-1)+ binary_map.at<uchar>(y+1,x+1);
				int cnt4 = binary_map.at<uchar>(y-1,x+1)+ binary_map.at<uchar>(y+1,x-1);

				if ((cnt1==2)||(cnt2==2)||(cnt3==2)||(cnt4==2)){
					for (int m=-1; m<2; m++)
						for (int n=-1; n<2; n++)
							binary_map3.at<uchar>(y+m,x+n) = 1;
				}
			}

		}

	for (int y = 4; y < height-4; y++ )
		for (int  x = 4; x < width-4; x++ ){
			int idx = y*width+x;
			if ((binary_map.at<uchar>(y,x) + binary_map3.at<uchar>(y,x))<2){
				confidence_x[idx]=0;
				confidence_y[idx]=0;
			}

		}

	//imwrite("test3.png", binary_map3*255);
	//imwrite("test2.png", binary_map2*255);// while(1);
}


/**
    Extact the depth from horizontal and vertical EPI slices
    @epi_h    horizontal EPI slices as input
    @epi_v    vertical   EPI slices as input
    @lf_ptr   the light field structure pointer
    @depth    the final result
*/
bool lf2depth(LF* lf_ptr){

    int width  = lf_ptr->W;
    int height = lf_ptr->H;
    int num_pixels = width*height;
    int num_labels = lf_ptr->nlabels;

    if (lf_ptr->type==0){ //HCI
        lf_ptr->d_min=lf_ptr->dt_min;
        lf_ptr->d_max=lf_ptr->dt_max;
    }
    if (fabs(lf_ptr->d_min-lf_ptr->d_max)>2)
        lf_ptr->nlabels=64;
    else
        lf_ptr->nlabels=64;
    num_labels = lf_ptr->nlabels;

    float *depth_x      = new float[num_pixels*num_labels];
    float *depth_y      = new float[num_pixels*num_labels];
    float *depth_cx     = new float[num_pixels*num_labels];
    float *depth_cy     = new float[num_pixels*num_labels];
    float *confidence_x = (float*) calloc (num_pixels, sizeof(float)); 
    float *confidence_y = (float*) calloc (num_pixels, sizeof(float)); 
    uchar *depth_best_x = (uchar*) calloc (num_pixels, sizeof(uchar)); 
    uchar *depth_best_y = (uchar*) calloc (num_pixels, sizeof(uchar)); 
    uchar *depth_best_xy  = new uchar[num_pixels];

    int64 t0, t1;
    t0 = cv::getTickCount();
    cost_volume(lf_ptr->epi_h, lf_ptr->epi_v, depth_x, depth_y, depth_cx, depth_cy, lf_ptr); //build the cost volume
    compute_slope_xy    (depth_x, depth_y, depth_cx, depth_cy, confidence_x, confidence_y, depth_best_xy, lf_ptr);//===xy estimate
    spatial_filtering(confidence_x, confidence_y, lf_ptr);


    t1 = cv::getTickCount();   
    cout<<"Time spent "<<(t1-t0)/cv::getTickFrequency()<<" Seconds"<<endl;  
    
    //=================================================================== 
    Mat dvx (  height*width*lf_ptr->nlabels, 1, CV_32F, depth_x);
    Mat dvy (  height*width*lf_ptr->nlabels, 1, CV_32F, depth_y);          
    //mat2hdf5("./debug/data/dvx.h5", "data", H5T_NATIVE_FLOAT, float(), dvx);                     
    //mat2hdf5("./debug/data/dvy.h5", "data", H5T_NATIVE_FLOAT, float(), dvy);    
    Mat rrr = Mat(height, width, CV_8U, depth_best_xy);
    //color_map_confidence(lf_ptr, rrr, "./debug/data/depth0.png", 0, confidence_x, confidence_y, 1); 
    //color_map_confidence(lf_ptr, rrr, "./debug/data/depth2.png", 2, confidence_x, confidence_y, 1);      
    //color_map_confidence(lf_ptr, rrr, "./debug/data/depth4.png", 4, confidence_x, confidence_y, 1);
    //color_map_confidence(lf_ptr, rrr, "./debug/data/depth6.png", 6, confidence_x, confidence_y, 1);

    if (lf_ptr->type==1){ //Refine the depth result for Lytro data
        lf2depth_mrf(depth_x, depth_y, confidence_x, confidence_y, lf_ptr);
    }
    else {//Just copy
        Mat result = Mat(height, width, CV_8U, depth_best_xy);
        result.convertTo(lf_ptr->depth, CV_32F);
    }
    depth_filtering(lf_ptr);//post filtering
    color_map(lf_ptr->depth(Rect(20,20,width-40,height-40)),  lf_ptr->depth_filename.c_str(),       0);
    color_map(lf_ptr->depth_f(Rect(20,20,width-40,height-40)),lf_ptr->depth_filter_filename.c_str(),0);
    //grey_map (lf_ptr->depth_f(Rect(20,20,width-40,height-40))*4,"./debug/data/r.png",0);
 
	delete[] depth_x;
	delete[] depth_y;
	delete[] depth_cx;
	delete[] depth_cy;	
	delete[] confidence_x;
	delete[] confidence_y;
	delete[] depth_best_x;
	delete[] depth_best_y;
	delete[] depth_best_xy;
    delete[] d;
	 
	return true;
}

#endif
