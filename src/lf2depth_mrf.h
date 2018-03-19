//  Refine the disparity map from the cost volume using multi-label optimization.

#ifndef _LF2DEPTH_MRF
#define _LF2DEPTH_MRF

#include <limits>
#include <opencv2/opencv.hpp>
#include "gco/GCoptimization.h"
#include "volume_filtering.h"
#include "light_field.h"
#include "misc.h"
using namespace std;
using namespace cv;

/**
    Merge the vertical and horizontal volumes to a single volume.
    @depth_x  the horizontal volume as input
    @depth_y  the vertical   volume as input
    @cost     the merged volume as output
    @lf_ptr   the light field structure pointer
*/
void volume_merge(  float* depth_x, 
                    float* depth_y,  
					float* conf_x,
					float* conf_y,              
                    float* cost, 
                    LF* lf_ptr){
                   
	for (int j = 1; j < (lf_ptr->H-1); j++)
		for (int i = 1; i < (lf_ptr->W-1); i++){
			int idx = j*lf_ptr->W +i;
			if (conf_x[idx]>conf_y[idx])
				memcpy(&cost[lf_ptr->nlabels*idx], &depth_x[lf_ptr->nlabels*idx], lf_ptr->nlabels*sizeof(float) );
		    else   
				memcpy(&cost[lf_ptr->nlabels*idx], &depth_y[lf_ptr->nlabels*idx], lf_ptr->nlabels*sizeof(float) );      
		}                      
}

/**
    Using Makov Random Field (Multi-label optimization) to refine the disparity map.
    @depth_x  the horizontal volume as input
    @depth_y  the vertical   volume as input
    @confidence_x   the horizontal confidence map
    @confidence_y   the vertical   confidence map 
    @depth_best_xy  the first estimate disparity map   
    @lf_ptr   the light field structure pointer
*/

bool lf2depth_mrf(  float *depth_x,
                    float *depth_y,
                    float *confidence_x,
                    float *confidence_y,              
                    LF* lf_ptr){

    int width  = lf_ptr->W;
    int height = lf_ptr->H;
    int num_pixels = width*height;
    int num_labels = lf_ptr->nlabels;
    float *cost         = new float[num_pixels*num_labels];

    cout<<" ======== MRF Refinement Result ======>>>>>>>"<<endl;
    volume_merge(depth_x, depth_y, confidence_x, confidence_y, cost, lf_ptr);
         
    int *smooth = new int[num_labels*num_labels];
	    for (int l1 = 0; l1 < num_labels; l1++)
		    for (int l2 = 0; l2 < num_labels; l2++)
			    smooth[l1 + l2*num_labels] = abs(l1 - l2);//abs(l1 - l2);//*(l1 - l2);//abs(l1 - l2);//l1 norm

    try{

	    GCoptimizationGeneralGraph *gc = new GCoptimizationGeneralGraph(num_pixels, num_labels);

        if (lf_ptr->lambda==0) lf_ptr->lambda=1;
	    //add data costs individually
	    for (int j = 0; j<height; j++)
		    for (int i = 0; i<width; i++){

		        int idx = j*width+i;
		        for (int k = 0; k<num_labels; k++){
		        
		        	if ((j>4)&&(i>4)&&(j<(width-4))&&(i<(height-4))){
		               		            
       					if ((confidence_x[idx]<lf_ptr->threshold)&&(confidence_y[idx]<lf_ptr->threshold))
		                    gc->setDataCost(idx, k, 0);				            
		                else
		                    gc->setDataCost(idx, k, (lf_ptr->lambda*cost[num_labels*idx+k])); 			 
			        }
			        else 
			            gc->setDataCost(idx, k, 0); 
			    }
		    }

		    // now set up a grid neighborhood system
		    // first set up horizontal neighbors
		    for (int y = 1; y < height; y++ )
			    for (int  x = 1; x < width; x++ ){
				    int p1  = x-1+y*width;
				    int p2  = x+  y*width;			
				    int p3  = x+(y-1)*width;										
				    //int  weightx = confidence_x[p2]>lf_ptr->threshold ? 0: 1;	
				    //int  weighty = confidence_y[p2]>lf_ptr->threshold ? 0: 1;						    
				    if (confidence_x[p2]<lf_ptr->threshold) gc->setNeighbors(p1,p2);
				    if (confidence_y[p2]<lf_ptr->threshold) gc->setNeighbors(p3,p2);		
			    }

	    gc->setSmoothCost(smooth);
        gc->setVerbosity(1);
	    printf("Before optimization energy is %lld\n", gc->compute_energy());
	    gc->expansion(1);
        printf("After optimization energy is %lld\n", gc->compute_energy());
	    for (int j = 0; j<height; j++)
		    for (int i = 0; i<width; i++)
			    lf_ptr->depth.at<float>(j,i) = gc->whatLabel(j*width+i);
      
	    delete gc;
    }

    catch (GCException e){
	    e.Report();
    }
        
    delete[] cost;
	delete[] smooth;	
	return true;
}

#endif

