// load light field data (HDF5) and scene cofiguration(XML) implementation.


#ifndef _CONFIG
#define _CONFIG

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

#include "h5_io.h"
#include "light_field.h"

using namespace std;
using namespace cv;

/**
    Load the scene configuration file.
    @filename        file name to load. Should be a xml file.
     @lf_ptr          light field strutue pointer 
*/
void config_read(string filename, LF* lf_ptr){
    
    //===load configuration file from xml file
    FileStorage fs;
    fs.open(filename, FileStorage::READ);
    FileNode n = fs["DEPTH_H5"]; 
    lf_ptr->disparity_filename= (string) *n.begin();
    n = fs["DATA_H5"]; 
    lf_ptr->data_filename= (string) *n.begin();
    n = fs["DEPTH_IMG"]; 
    lf_ptr->depth_filename= (string) *n.begin();
    n = fs["DEPTH_IMG_FILTER"]; 
    lf_ptr->depth_filter_filename= (string) *n.begin();
    n = fs["ERROR_IMG"]; 
    lf_ptr->erro_map_filename = (string) *n.begin();
    n = fs["CVIEW"]; 
    lf_ptr->centre_view_filename =(string) *n.begin();

   
    lf_ptr->W = fs["WW"]; lf_ptr->H = fs["HH"];
    lf_ptr->U = fs["AA"]; lf_ptr->V = fs["AA"];
    lf_ptr->d_min=fs["DMIN"];
    lf_ptr->d_max=fs["DMAX"];
    lf_ptr->type  = fs["DATASET"];
    lf_ptr->conf = fs["CONFIDENCE"]; 
    lf_ptr->mask = fs["MASK"]; 
    lf_ptr->lambda = fs["LAMBDA"];
    lf_ptr->threshold = fs["THRESHOLD"];    
    //if (lf_ptr->nlabels==0)
        lf_ptr->nlabels = fs["NUM_LABELS"]; 
    fs.release();
}

/**
    Load light field data and attritubes from HCI data.
    @lf_ptr          light field strutue pointer 
*/
void loadh5_mat(LF* lf_ptr){
    
    lf_ptr->img = Mat(lf_ptr->H*lf_ptr->U, lf_ptr->W*lf_ptr->V, CV_8UC3);  
    //medianBlur ( lf_ptr->img, lf_ptr->img, 3 );
    lf_ptr->lf_raw = new unsigned char[lf_ptr->H*lf_ptr->W*3*lf_ptr->V*lf_ptr->U];

    //load camera setup
    lf_ptr->shift       = 0.0;
    lf_ptr->baseline    = 0.0;
    lf_ptr->focalLength = 0.0;       
 
    load_hdf5_attri(lf_ptr->data_filename.c_str(), lf_ptr->shift, lf_ptr->baseline, lf_ptr->focalLength);
    //cout<<shift<<" "<<baseline<<" "<<focalLength<<endl;
    
    //load data
    load_hci_hdf5(lf_ptr->data_filename.c_str(), "LF", H5T_NATIVE_UCHAR, (uchar*) lf_ptr->lf_raw); 
    int cnt=0;
        for (int v = 0; v< lf_ptr->V; v++)    
            for (int u = 0; u< lf_ptr->U; u++)
                for (int j=0; j<lf_ptr->H; j++) { 
                    for (int i=0; i<lf_ptr->W; i++) { 
                        //int idx = (v*lf_ptr->U+u)*lf_ptr->W*lf_ptr->H*3 + 3*lf_ptr->H*j + 3*i;
                        lf_ptr->img.at<Vec3b>(v*lf_ptr->H+j,u*lf_ptr->W+i)
                        = Vec3b( lf_ptr->lf_raw[cnt+2],  lf_ptr->lf_raw[cnt+1],  lf_ptr->lf_raw[cnt]);
                        cnt=cnt+3;                        
                    }
                }

    //load the ground truth           
    lf_ptr->disparity_gt = Mat(lf_ptr->H,lf_ptr->W,CV_32F);         
    float* lf_gt= new float[lf_ptr->H*lf_ptr->W*lf_ptr->V*lf_ptr->U];            
    load_hci_hdf5(lf_ptr->data_filename.c_str(), "GT_DEPTH", H5T_NATIVE_FLOAT, (float*) lf_gt);  
    //take the central view depth            
    for (int j=0; j<lf_ptr->H; j++) 
       for (int i = 0; i < lf_ptr->W; i++){                
          int idx =((lf_ptr->V*lf_ptr->U-1)/2)*lf_ptr->W*lf_ptr->H + lf_ptr->W*j+i;           
          lf_ptr->disparity_gt.at<float>(j,i) = lf_ptr->baseline * lf_ptr->focalLength / lf_gt[idx] - lf_ptr->shift;
        }
    //cout<<"========================"<<endl;
    
    //load the mask if avaliable
    if (lf_ptr->mask==1){
        float* lf_mask= new float[lf_ptr->H*lf_ptr->W*lf_ptr->V*lf_ptr->U];            
        load_hci_hdf5(lf_ptr->data_filename.c_str(), "GT_DEPTH_MASK", H5T_NATIVE_FLOAT, (float*) lf_mask);  
        //take the central view depth            
        for (int j=0; j<lf_ptr->H; j++) 
           for (int i = 0; i < lf_ptr->W; i++){                
              int idx =((lf_ptr->V*lf_ptr->U-1)/2)*lf_ptr->W*lf_ptr->H + lf_ptr->W*j+i;           
              lf_ptr->disparity_mask.at<float>(j,i) = lf_mask[idx];
            }
        delete[] lf_mask;         
    }    
        
   //Clamp disparity to global range 
        for (int j=0; j<lf_ptr->H; j++) { 
            for (int i=0; i<lf_ptr->W; i++) {   
          
               if (lf_ptr->disparity_mask.at<float>(j,i)>0){
                    //remove ouliter by the blender
                    float d = lf_ptr->disparity_gt.at<float>(j,i);
                    if ( d > lf_ptr->d_max || d < lf_ptr->d_min ) {
                        if ((i+j)==0)
                            lf_ptr->disparity_gt.at<float>(j,i) = lf_ptr->disparity_gt.at<float>(j,i+1);
                        else if ((i==(lf_ptr->W-1))&&(j==(lf_ptr->H-1)))
                            lf_ptr->disparity_gt.at<float>(j,i) = lf_ptr->disparity_gt.at<float>(j,i-1);        
                        else
                            lf_ptr->disparity_gt.at<float>(j,i) =
                            0.5*lf_ptr->disparity_gt.at<float>(j,i+1)+
                            0.5*lf_ptr->disparity_gt.at<float>(j,i-1);
                    }
                }
                	
            }
        }     
  
       lf_ptr->dt_min =  lf_ptr->disparity_gt.at<float>(0,0);
       lf_ptr->dt_max =  lf_ptr->disparity_gt.at<float>(0,0);

        for (int j=0; j<lf_ptr->H; j++) { 
            for (int i=0; i<lf_ptr->W; i++) {   
                lf_ptr->dt_min = min( lf_ptr->dt_min, lf_ptr->disparity_gt.at<float>(j,i) );
                lf_ptr->dt_max = max( lf_ptr->dt_max, lf_ptr->disparity_gt.at<float>(j,i) );
            }
        }
       //lf_ptr->d_min=lf_ptr->d_min-0.1;
       //lf_ptr->d_max=lf_ptr->d_max+0.1;       
        cout<<lf_ptr->dt_min<<" "<<lf_ptr->dt_max<<endl;      
               
    //find max and min depth    
    //delete[] lf;
    delete[] lf_gt;        
}

#endif




