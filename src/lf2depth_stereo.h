//lf2depth_stereo.h 

#ifndef _DEPTH_FROM_LIGHTFIELD_STEREO
#define _DEPTH_FROM_LIGHTFIELD_STEREO
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
/**
    Bilinear interpolation
    @a1   a2   b1  b2  four nodes
    @alpha beta        coefficients
*/
float bilinear(uchar a1, uchar a2, uchar b1, uchar b2, float alpha, float beta ){

float value = float(a1)*alpha*beta     + float(b1)*alpha*(1-beta) 
            + float(a2)*(1-alpha)*beta + float(b2)*(1-alpha)*(1-beta);

return value;
}


/**
    Extact the depth from  multiple view stereo
    @epi_h    horizontal EPI slices as input
    @epi_v    vertical   EPI slices as input
    @lf_ptr   the light field structure pointer
    @depth    the final result
*/
bool lf2depth_stereo(LF* lf_ptr){

    size_t W = lf_ptr->W;
    size_t H = lf_ptr->H;
    size_t N = 64;

    //float *stack = new float[ W*H*N ];
    uchar *depth = new uchar[ W*H];    

    int s1 = 4;
    int t1 = 4;

    float *d  = new float[lf_ptr->nlabels+1];   
    for (int k=0; k<=lf_ptr->nlabels; k++){
        if (lf_ptr->type==0)
	        d[k]= lf_ptr->dt_min+(float)k*float(lf_ptr->dt_max-lf_ptr->dt_min)/((float)lf_ptr->nlabels);
	    else
	        d[k]= lf_ptr->d_min+(float)k*float(lf_ptr->d_max-lf_ptr->d_min)/((float)lf_ptr->nlabels);
	    //cout<<lf_ptr->nlabels<<"=== "<<lf_ptr->dt_min<<" "<<lf_ptr->dt_max<<" "<<d[k]<<endl;
    }
  
    #pragma omp parallel for
    for ( size_t y=0; y<H; y++ ) {

        for ( size_t x=0; x<W; x++ ) {

            int idxopt = 32;
            double eopt = 1e10;
            float data[3];  //get the central view data;

            if (lf_ptr->type==0){     
                data[0]= (float) lf_ptr->lf_raw[ (t1*lf_ptr->U+s1)*W*H*3 + 3*y*W + 3*x];
                data[1]= (float) lf_ptr->lf_raw[ (t1*lf_ptr->U+s1)*W*H*3 + 3*y*W + 3*x +1];
                data[2]= (float) lf_ptr->lf_raw[ (t1*lf_ptr->U+s1)*W*H*3 + 3*y*W + 3*x +2];   
            }       
            else{
                data[0]= lf_ptr->img.at<Vec3b>(t1*lf_ptr->H+y,s1*lf_ptr->W+x)[0];
                data[1]= lf_ptr->img.at<Vec3b>(t1*lf_ptr->H+y,s1*lf_ptr->W+x)[1];
                data[2]= lf_ptr->img.at<Vec3b>(t1*lf_ptr->H+y,s1*lf_ptr->W+x)[2];                
            }
            for ( size_t i=0; i<N; i++ ) {
 
                float err = 0.0f;

	            for ( int t=1; t<(lf_ptr->U-1); t++ ) {
	                for ( int s=1; s<(lf_ptr->V-1); s++ ) {
 
	                    float xnew = float(x) + d[i] * float( s - s1 );
	                    float ynew = (lf_ptr->type==0) ? float(y) - d[i] * float( t - t1 ) :  float(y) + d[i] * float( t - t1 );

	                    if ( xnew>=0 && xnew<=W-1 && ynew>=0 && ynew<=H-1 ) {// abs(s-4)>0
	                 
	                      float tmp[3];
	                      float data_new[3];//get view data;

	                      int idx  = (t*lf_ptr->U+s)*W*H*3 + 3*int(ynew)*W   + 3*int(xnew);
		                  int idx2 = idx+ 3*W;              
	                      	                      
	                      float alpha = 1.0f-(xnew-int(xnew));
	                      float beta  = 1.0f-(ynew-int(ynew));	                      
	                      
	                      if (lf_ptr->type==0){                              
	                      data_new[0]=bilinear( lf_ptr->lf_raw[idx+0], lf_ptr->lf_raw[idx+3+0], 
	                                            lf_ptr->lf_raw[idx2+0],lf_ptr->lf_raw[idx2+3+0], alpha, beta);
                          data_new[1]=bilinear( lf_ptr->lf_raw[idx+1], lf_ptr->lf_raw[idx+3+1], 
                                                lf_ptr->lf_raw[idx2+1],lf_ptr->lf_raw[idx2+3+1], alpha, beta);
                          data_new[2]=bilinear( lf_ptr->lf_raw[idx+2], lf_ptr->lf_raw[idx+3+2], 
                                                lf_ptr->lf_raw[idx2+2],lf_ptr->lf_raw[idx2+3+2], alpha, beta);    
                          //cout<<alpha<<" "<<beta<<" "<<data_new[0]<<" "<<(float)lf_ptr->lf_raw[idx+0]<<endl;  
                          }
                          else{
                          data_new[0]= bilinear( lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew),s*lf_ptr->W+int(xnew))[0], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew),s*lf_ptr->W+int(xnew+1))[0], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew+1),s*lf_ptr->W+int(xnew))[0], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew+1),s*lf_ptr->W+int(xnew+1))[0],
	                                             alpha, beta);
                          data_new[1]= bilinear( lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew),s*lf_ptr->W+int(xnew))[1], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew),s*lf_ptr->W+int(xnew+1))[1], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew+1),s*lf_ptr->W+int(xnew))[1], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew+1),s*lf_ptr->W+int(xnew+1))[1],
	                                             alpha, beta);                        
                          data_new[2]= bilinear( lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew),s*lf_ptr->W+int(xnew))[2], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew),s*lf_ptr->W+int(xnew+1))[2], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew+1),s*lf_ptr->W+int(xnew))[2], 
                                                 lf_ptr->img.at<Vec3b>(t*lf_ptr->H+int(ynew+1),s*lf_ptr->W+int(xnew+1))[2],
	                                             alpha, beta);                        
                          }               
		                  tmp[0] = data[0] -data_new[0];//norm(data -data_new); 
		                  tmp[1] = data[1] -data_new[1];//norm(data -data_new);  
		            	  tmp[2] = data[2] -data_new[2];//norm(data -data_new);                                          
		                  err += tmp[0]*tmp[0]+tmp[1]*tmp[1]+tmp[2]*tmp[2];	
		                  //err=data_new[2];
	                    }
                    }
               }
               if ( err<eopt ) {
                    //cout<<err<<endl;
	                eopt   = err;
	                idxopt = i;
	           }
            }
            depth[y*W+x] = idxopt;
        }
    }

    if (lf_ptr->type==0)
        evaluate_depth  (depth,  "./debug/data/depth_stereo.png",   lf_ptr);
    else{
         Mat img_grey;
         cvtColor(lf_ptr->imgc, img_grey, CV_RGB2GRAY);
         JointWMF wmf;
         
         Mat depth_mat2( lf_ptr->imgc.cols, lf_ptr->imgc.rows, CV_8U, depth);
         imwrite("./debug/data/a.png",depth_mat2(Rect(20,20,600,600)));        
         Mat depth_mat ( lf_ptr->imgc.rows, lf_ptr->imgc.cols, CV_32F);
         label2depth( depth_mat2, depth_mat, lf_ptr);
         Mat depth_f=wmf.filter(depth_mat, img_grey, 5, 25.5, 64, 256, 1, "exp");//cos
         color_map(depth_mat2(Rect(20,20,lf_ptr->imgc.cols-40,lf_ptr->imgc.rows-40)), "./debug/data/depth_stereo.png",  0);
    }
	return true;
}

#endif
