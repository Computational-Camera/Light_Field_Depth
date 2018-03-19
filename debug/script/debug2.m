clear;
close all;
clc;


y=558+1;
x=304+1;

d_min=-0.845765;
d_max=1.54017;

load gt.h5
gt=data;
value_gt=64*(gt(x,y)-d_min)/(d_max-(d_min));
display(value_gt);
display(2*(d_max-d_min)/64);
load debugx.h5;
debugx=data;
load debugy.h5;
debugy=data;

w=768;

dataxr=debugx(:,(y-1)*w+x);
datayr=debugy(:,(y-1)*w+x);
dataxg=debugx(:,(y-1)*w+x + w*w);
datayg=debugy(:,(y-1)*w+x + w*w);
dataxb=debugx(:,(y-1)*w+x + 2*w*w);
datayb=debugy(:,(y-1)*w+x + 2*w*w);

%delete data;
%delete debugx;
%delete debugy;
sum_data1= dataxr + dataxg + dataxb + datayr + datayg + datayb;


plot(dataxr, 'r');
hold on;
plot(dataxg, 'g');
hold on;
plot(dataxb, 'b');
hold on;

plot(datayr, 'r*');
hold on;
plot(datayg, 'g*');
hold on;
plot(datayb, 'b*');

hold on;
plot([value_gt,value_gt],[0,max(sum_data1)/6],'r');

figure;




plot(sum_data1, 'b*');
hold on;
plot([value_gt,value_gt],[0,max(sum_data1)/6],'r');
hold on;
[value,est]=min(sum_data1(:));
plot([est,est],[0,max(sum_data1)/6],'r');
figure;
sum_data2= dataxr.*dataxg.*dataxb.*datayr.*datayg.*datayb;
plot(sum_data2, 'b*');



