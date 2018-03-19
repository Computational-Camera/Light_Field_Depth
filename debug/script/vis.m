clear all;
clc;
close all;

load gt.h5;
gt = data';
gt = gt(285,6:760);
plot(gt,'k');

load dx.h5;
dx = medfilt1(data(6:760,285)',5);
load d.h5;
d = medfilt1(data(6:760,285)',5);
load dy.h5;
dy = medfilt1(data(6:760,285)',5);


figure;
subplot (3,1,1);
plot(dx-gt,'k','LineWidth',2);
axis ([0, 600, -0.5, 0.5]);
xlabel('Horizontal Result','FontSize', 12);
ylabel('Error in pixels','FontSize', 12);
subplot (3,1,2);
plot(dy-gt,'k','LineWidth',2);
axis ([0, 600, -0.5, 0.5]);
xlabel('Vertical Result','FontSize', 12);
ylabel('Error in pixels','FontSize', 12);
subplot (3,1,3);
plot(d-gt,'k','LineWidth',2);
axis ([0, 600, -0.5, 0.5]);
xlabel('Fusion Result','FontSize', 12);
ylabel('Error in pixels','FontSize', 12);

print -dsvg  "-S1100,500" fusion.svg


for i=1:755

    e1 = abs(dx(i)-gt(i));
    e2 = abs(dy(i)-gt(i));

    if (e1<e2)
        dd(i) = dx(i);
    else
        dd(i) = dy(i);
    end
end


figure;
load cx.h5;
cx = data';
plot(cx(285,6:760),'r');

load cy.h5;
cy = data';
hold on;
plot(cy(285,6:760),'g');
