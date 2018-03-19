clear;
clc;

load dx.h5;
dx = data';
load dy.h5;
dy = data';
load cx.h5;
cx = data';
load cy.h5;
cy = data';

s = size(data);

threshold = 4;

for j = 1: s(1)
    for i = 1: s(2)
   
        if ((cx(i,j)>threshold)&&(i>4)&&(j>4)&&(i<(s(1)-4))&&(j<(s(1)-4)))
            dx(i,j) = dx(i,j);
        else 
            dx(i,j) = 0;
        end

        if ((cy(i,j)>threshold)&&(i>4)&&(j>4)&&(i<(s(1)-4))&&(j<(s(1)-4)))
            dy(i,j) = dy(i,j);
        else 
            dy(i,j) = 0;
        end
       
       if (cy(i,j)>cx(i,j))
           dd(i,j)=dy(i,j);
       else
           dd(i,j)=dx(i,j);  
       end
      
    end
end
figure;
imagesc(dx);
figure;
imagesc(dy);
figure;
imagesc(dd);
