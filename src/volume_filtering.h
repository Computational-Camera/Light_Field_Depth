//  3D convolution with box filter implementation.

#ifndef _VOLUME_FILTERING
#define _VOLUME_FILTERING
/**
    Volume filtering (3D convolution), both input and output 3D data buffer
    need to be pre allocated outside the function.
    @data_ptr        input data buffer pointer
    @blur_ptr        output data buffer pointer pointer
    @w               width
    @h               height
    @l               length
*/
bool volume_filtering( float* data_ptr, float* blur_ptr, int w, int h, int l){
        //cout<<"----------"<<endl;
	    float*    cur_ptr;
	    float*    next_ptr;
	    float*    pre_ptr;
	    float*    result_ptr;

	    int block=w*h;
	    /* x axix */      
	    for(int z=1;z<l-1;z++)
	    { for(int y=1;y<h-1;y++)
	      {   
		      int offset = z*block+y*w+1;
		      cur_ptr    = data_ptr+offset; 
		      pre_ptr    = cur_ptr-1;
		      next_ptr   = cur_ptr+1;
		      result_ptr = blur_ptr+offset; 

		      for(int x=1;x<w-1;x++)
		      {     
			     *(result_ptr)=(2*(*(cur_ptr))+*next_ptr+*pre_ptr)/4;
			     //cout<<*(result_ptr)<<" "<<*(cur_ptr)<<endl;
			     result_ptr++;
			     cur_ptr++;
			     next_ptr++;
			     pre_ptr++;
		      }
	       }
	    }


        /* y axix */                        
	    for(int z=1;z<l-1;z++)
	    { for(int x=1;x<w-1;x++)
	      {  
		      int offset=z*block+w+x;
		      cur_ptr =blur_ptr+offset;
		      pre_ptr =cur_ptr-w;
		      next_ptr=cur_ptr+w;
		      result_ptr=data_ptr+offset;

		      for(int y=1;y<h-1;y++)
		      {  
			     *(result_ptr)=(2*(*(cur_ptr))+*next_ptr+*pre_ptr)/4; 
			     result_ptr= result_ptr+w;
			     pre_ptr   = cur_ptr; 
			     cur_ptr   = next_ptr;
			     next_ptr  = cur_ptr+w;
             } 
        }
       }

	    /* z axix */      
	    for(int y=1;y<h-1;y++)
	    { for(int x=1;x<w-1;x++)
	      {   
		      int offset=block+y*w+x;
		      cur_ptr   =data_ptr+offset;
		      pre_ptr   =cur_ptr-block;
		      next_ptr  =cur_ptr+block;
		      result_ptr=blur_ptr+offset;   
	        
		      for(int z=1;z<l-1;z++)
		      {       
			     *(result_ptr)=(2*(*(cur_ptr))+*next_ptr+*pre_ptr)/4; 
			     //cout<<*(result_ptr)<<" "<<*(cur_ptr)<<endl;
			     result_ptr=result_ptr+block;
			     pre_ptr=cur_ptr; 
			     cur_ptr=next_ptr;
			     next_ptr=cur_ptr+block;
		      }
	      }
	    }
    return true;
}

#endif
