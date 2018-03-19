/* 
##############################################################################
#                                                                            #
#    LF2DEPTH - Extract depth from Light Field data                          #
#                        Version 1.1                                         #
#    Author Shan Xu xushan2011@gmail.com                                     #
#    Copyright 2013-2016                                                     #
#                                                                            #
##############################################################################


    To use this software, YOU MUST CITE the following publication:

    A Simple Framework for Extracting Accurate Depth Map from Lenslet-based Light-Field Cameras 
**  
**    Copyright (C) 2012-2016 
**  LF2DEPTH is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
    
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
   
**  You should have received a copy of the GNU General Public License
**  along with this program in the file "LICENSE".
**  If not, see <http://www.gnu.org/licenses/>.
**
 */
 
#include "config.h"
#include "light_field.h"
#include "lf2depth.h"
#include "lf2depth_stereo.h"
#include "misc.h"

using namespace std;
using namespace cv;
/**
    Initialize light field structure and load data.
    @lf_ptr          light field strutue pointer 
*/
void lf_init(LF* lf_ptr){

    lf_ptr->depth         =Mat::zeros(lf_ptr->H,lf_ptr->W,CV_32F);
    lf_ptr->depth_f       =Mat::zeros(lf_ptr->H,lf_ptr->W,CV_32F);
    lf_ptr->depth_x       =Mat::zeros(lf_ptr->H,lf_ptr->W,CV_32FC3);
    lf_ptr->depth_y       =Mat::zeros(lf_ptr->H,lf_ptr->W,CV_32FC3);
    lf_ptr->confidence    =Mat::zeros(lf_ptr->H,lf_ptr->W,CV_32F);
    lf_ptr->confidence_x  =Mat::zeros(lf_ptr->H,lf_ptr->W,CV_32FC3);
    lf_ptr->confidence_y  =Mat::zeros(lf_ptr->H,lf_ptr->W,CV_32FC3);  

    if (lf_ptr->mask==1)
        lf_ptr->disparity_mask = Mat::zeros(lf_ptr->H,lf_ptr->W,CV_32F);
    else
        lf_ptr->disparity_mask = Mat::ones( lf_ptr->H,lf_ptr->W,CV_32F);  

    if (lf_ptr->type==0)//HCI data
        loadh5_mat(lf_ptr);
    else//Lytro dat   
        lf_ptr->img=imread(lf_ptr->data_filename.c_str()); 
      
    mview2epis(lf_ptr->epi_h, lf_ptr->epi_v, lf_ptr);//convert multiview to EPI slices  
}


//==================Main Function===========================
//command usage ./bin/depth ./config/HCI/papillon.xml
//==========================================================

int main(int argc, const char *argv[]) {
    cout<<"=================Start====================  "<<argv[1]<<endl;
    LF lf;

    config_read(argv[1], &lf); //load xml configuration file
    lf_init(&lf);
    lf2depth(&lf);//Depth extraction

    //int64 t0, t1;
    //t0 = cv::getTickCount();
    //cout<<" ======== Multiview     ======>>>>>>>"<<endl;
    //lf2depth_stereo(&lf);
    //t1 = cv::getTickCount();   
    //cout<<"Time spent "<<(t1-t0)/cv::getTickFrequency()<<" Seconds"<<endl;  

    if (lf.type==0)
    delete[] lf.lf_raw;

    cout<<"=================Finish===================="<<endl;
    return 0;
}
