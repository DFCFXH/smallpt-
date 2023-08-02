/* smallpt,һ��С��·��׷���� */

/*
·��׷�ٵĻ���˼���Ǵ��ӵ㷢��һ������,��������������ཻʱ���ݱ���Ĳ������Լ�������һ������,������һ������,���
����,ֱ�����ߴ򵽹�Դ��(�����ݳ�����),Ȼ�������ؿ���ķ���,�����乱��,��Ϊ���ص���ɫֵ.
·��׷�ٻ�ܿ�����С��·��,���ڹ��״��·������������ֲ���̽��.
·��׷��=����׷��+���ؿ��巽��
*/

#define _CRT_SECURE_NO_WARNINGS //����fopen�ľ���
#define _USE_MATH_DEFINES 
#include <math.h>
#include <stdlib.h> 
#include <stdio.h>
#include <omp.h>
#define float2 double

//#define IMAGE_PATHNAME "image.ppm"
double erand48(unsigned short xsubi[3]) { //�����
    return (double)rand() / (double)RAND_MAX;
}
struct Vec { //��ά����
    double x, y, z; //λ�û���ɫ����ʹ��
    Vec(double x_ = 0, double y_ = 0, double z_ = 0) { x = x_; y = y_; z = z_; } //���캯��,x,y,zĬ��Ϊ��
    Vec operator+(const Vec& b) const { return Vec(x + b.x, y + b.y, z + b.z); }
    Vec operator-(const Vec& b) const { return Vec(x - b.x, y - b.y, z - b.z); }
    Vec operator*(double b) const { return Vec(x * b, y * b, z * b); } //�����˱���
    Vec mult(const Vec& b) const { return Vec(x * b.x, y * b.y, z * b.z); } //����������,������ɫ����
    Vec& norm() { return *this = *this * (1 / sqrt(x * x + y * y + z * z)); } //��һ��
    double dot(const Vec& b) const { return x * b.x + y * b.y + z * b.z; } //���
    Vec operator%(Vec& b) { return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }//���
    Vec operator/(const Vec& b) const { return Vec(x / b.x, y / b.y, z / b.z); }
    double DIV(const double& b) const { return x /b + y / b + z / b;  }//�������Ա���
};
struct Ray { Vec o, d; Ray(Vec o_, Vec d_) : o(o_), d(d_) {} }; //���߽ṹ��
enum obj_type { S, P, T,BVH }; //��������
struct Obj {
    double rad, ref, diff, spec, refr, refr_nt,w,h,size; //�뾶,����,������,���淴��,�������,������,��,��
    Vec p, e, c, i, n,p1,p2,p3; //λ��,�Է���,��ɫ(�������ɫ������0-255,����0-1),����,����
    obj_type type; // ��������
    Obj(double rad_, double ref_, double diff_, double spec_, double refr_, double refr_nt_,Vec i_,Vec p_, Vec e_, Vec c_,
        obj_type refl_,double w_,double h_,Vec n_,double size_,Vec p1_,Vec p2_,Vec p3_) ://���캯��
        rad(rad_), ref(ref_), diff(diff_), spec(spec_), refr(refr_), refr_nt(refr_nt_),i(i_), p(p_), e(e_), c(c_), type(refl_),w(w_),h(h_),n(n_)
    ,size(size_),p1(p1_), p2(p2_), p3(p3_) {}
    double intersect_sphere(const Ray& r) const { //��������ԭ��������֮��Ľ���ľ���,���û�н��㷵��0
        Vec op = p - r.o; //��Դָ�����ĵ�һ������
        double t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
        //eps��һ����С����,��ָ0
        //t�������뽻��֮��ľ���,�Ƿ���t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0�Ľ�"."��ʾ���
        //b�ǹ�Դָ�����ĵ������͹�Դ���������ļнǵ�����ֵ
        //detû�н�(<0)��û���ཻ^^^|op-t|=|r|         (op-t)^2-r^2=0
        if (det < 0) return 0; else det = sqrt(det);
        return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
        //����tֵ,���Ϊ0,���ʾ���ཻ
        //ѡ�����н�С�Ҵ���0�Ľ�,���det<0���޽�,��ʾ���ཻ,����0
    };
    double intersect_plane(const Ray& r) const {
        double t = n.dot(p - r.o) / n.dot(r.d);
        double dn = n.dot(r.d);

        //�������ƽ���ڻ�λ��ƽ����棬��û���ཻ
        if (fabs(dn) < 1e-6 || dn >= 0) {
            return 0;
        }

        //����
        Vec intersection = r.o + r.d * t;

        //��齻���Ƿ���ƽ�淶Χ��
        double half_width = w / 2.0;
        double half_height = h / 2.0;
        double z_distance = intersection.z - p.z; //���㽻�㵽ƽ�����ĵ�Z�����

        if (intersection.x < p.x - half_width || intersection.x > p.x + half_width ||
            intersection.y < p.y - half_height || intersection.y > p.y + half_height ||
            z_distance < 0 || z_distance > h) { //��齻����Z�᷽�����Ƿ��ڷ�Χ��
            return 0;
        }

        //�����ཻ�������ԭ��֮��ľ���
        return t;
    };
    double intersect_triangle(const Ray& r) const { //��������ԭ��������֮��Ľ���ľ���,���û�н��㷵��0
        /*
		Vec p2_ = p2 - p1;
        Vec p3_ = p3 - p1;
        Vec p1 = p;
        Vec p2 = p1 + p2_;
        Vec p3 = p1 + p3_;
        Vec nl = n;
        if (n.dot(r.d) > 0) nl=n * -1;
        if (fabs(nl.dot(r.d)) < -1e4) return 0;
        double t = (nl.dot(p1) - r.o.dot(nl)) / r.d.dot(nl);
        if (t < 0.0005f) return 0;
        Vec P = r.o +r.d*t;
        Vec a, b;
        a = p2 - p1;
        b = P - p1;
        Vec c1 = a%b;
        a = p3 - p2;
        b = P - p2;
        Vec c2 = a % b;
        a = p1 - p3;
        b = P - p3;
        Vec c3 = a % b;
        if (c1.dot(n) < 0 || c2.dot(n) < 0 || c3.dot(n) < 0) return 0;
        return t;
		*/
        const double EPSILON = 0.0000001; //Ҫ�Ƚϵ�Сֵ
        Vec p1_ = p1 * size + p;
        Vec p2_ = p2 * size + p;
        Vec p3_ = p3 * size + p;
        Vec p2 = p + p2_;
        Vec p3 = p + p3_;
        Vec p1 = p + p1_;
        Vec edge1 = p2 - p1;
        Vec edge2 = p3 - p1;
        Vec rd = r.d;
        Vec h = rd % edge2;
        double a = edge1.dot(h);
        Vec AB = p2 - p1, AC = p3 - p1, n0 = AB % AC;
        Vec n = n0.norm();
        double tdn = n.dot(r.d);
        if (a > -EPSILON && a < EPSILON || tdn >= 0)
            return 0.0; //����ƽ����������

        double f = 1.0 / a;
        Vec s = r.o - p1;
        double u = f * s.dot(h);

        if (u < 0.0 || u > 1.0)
            return 0.0; //������������֮��

        Vec q = s % edge1;
        double v = f * r.d.dot(q);

        if (v < 0.0 || u + v > 1.0)
            return 0.0; //������������֮��

        double t = f * edge2.dot(q);

        if (t > EPSILON)
            return t; //�ҵ�����

        return 0.0; //δ�ҵ�����
    };
};
/*
�뾶,�������,���������,���淴�����,�������,������,����,λ��,�Է���,��ɫ,����,��,����,��С,����λ��
*/
Obj scenes[] = {
      Obj(1e5,100,50,0,50,1.5,Vec(), Vec(1e5 + 1,40.8,81.6), Vec(),Vec(.75,.25,.25),S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //��ǽ
  Obj(1e5, 100,10,900,0,0,Vec(),Vec(-1e5 + 99,40.8,81.6),Vec(),Vec(.25,.25,.75),S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //��ǽ
  Obj(1e5,100,100,0,0,0, Vec(),Vec(50,40.8, 1e5),     Vec(),Vec(.75,.75,.75),S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //��ǽ
  Obj(1e5,100,100,0,0,0, Vec(),Vec(50,40.8,-1e5 + 170), Vec(.45,.45,.45),Vec(), S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //ǰǽ
  Obj(1e5, 100,0,100,0,0,Vec(),Vec(50, 1e5, 81.6),    Vec(),Vec(.75,.75,.75),S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //�ذ�
  Obj(1e5,100,100,0,0,1.5, Vec(),Vec(50,-1e5 + 81.6,81.6),Vec(),Vec(.75,.75,.75),S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //�컨��
  Obj(16.5,100,0,100,0,0,Vec(),Vec(27,16.5,47),       Vec(),Vec(1,1,1) * .999, S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //����
  Obj(16.5,100,16,0,84,1.5,Vec(),Vec(73,16.5,78),       Vec(),Vec(1,1,1) * .999, S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //�ֲڲ���
  Obj(600,100,100,0,0,0, Vec(),Vec(50,81.6 - .27,81.6),Vec(12,12,12),  Vec(), P,
  30,30,Vec(0,-1,0)
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //�Է���
  Obj(15,100,100,0,0,1.5,Vec(),Vec(50,50,81.6),       Vec(),Vec(1,1,1) * .999, S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //������
  Obj(10,100,0,0,100,1.5,Vec(.1,.1,.1),Vec(50,10,81.6),       Vec(),Vec(1,1,1) * .999, S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //����
  Obj(10,100,0,100,0,1.5,Vec(),Vec(50,30,43),       Vec(),Vec(1,1,1) * .999, P,
  20,20,Vec(0,0,1)
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //ƽ��
  Obj(13,100,0,10,90,1.5,Vec(),Vec(20,45,81.6),       Vec(),Vec(1,1,1) * .999, S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //���ⲣ����
  Obj(13,100,0,0,100,1.5,Vec(),Vec(80,47,81.6),       Vec(),Vec(1,1,1) * .999, S,
  0,0,Vec()
  ,1,Vec(1,1,1),Vec(2,2,2),Vec(3,3,3)), //������
  Obj(16.5,100,0,0,100,3,Vec(.1,.1,.1),Vec(25,15,25),Vec(),Vec(1,1,1) * .999, T,
  0,0,Vec()
  ,1,Vec(0,-20,50),Vec(40,20,50),Vec(-40,20,50)), //������
};
//���������������Ⱥ���ɫ����ĸ�������
inline double clamp(double x) { return x < 0 ? 0 : x>1 ? 1 : x; } //���ھ����ݹ���Ӻ����1����Ϊ1,С��0����Ϊ0
inline int toInt(double x) { return int(pow(clamp(x), 1 / 2.2) * 255 + .5); } // ��0 - 1ת��Ϊrgb�е�0 - 255, �����˸�1 / 2.2�ĵ���ֵ,�û������
//������
inline bool intersect(const Ray& r, double& t, int& id) { //t��ʾ����
    double n = sizeof(scenes) / sizeof(Obj), d, inf = t = 1e20;
    for (int i = int(n); i--;) { //����������
        if (scenes[i].type == S) {
            if ((d = scenes[i].intersect_sphere(r)) && d < t) {
                t = d;
                id = i;
            }
        }
        else if (scenes[i].type == P) {
            if ((d = scenes[i].intersect_plane(r)) && d < t) {
                t = d;
                id = i;
            }
        }
        else if (scenes[i].type == T) {
            if ((d = scenes[i].intersect_triangle(r)) && d < t) {
                t = d;
                id = i;
            }
        }
    }
    return t < inf; //���t�Ƿ����,���t������û���ཻ,����false,���򷵻�true
}
double ncg = 1; //����������
int numSpheres = sizeof(scenes) / sizeof(Obj);
Vec radiance(const Ray& r, int depth, unsigned short* Xi,int E=1) {
    double t; // �ཻ����
    int id = 0; // �ཻ�����ID
    if (!intersect(r, t, id)) return Vec(); // δ�ཻ�򷵻غ�ɫ
    const Obj& obj = scenes[id]; //�����еĶ���
    Vec x = r.o + r.d * t,  f = obj.c;
    Vec n,pn=obj.n;
    Vec AB = obj.p2 - obj.p1, AC = obj.p3 - obj.p1, n0_t = AB % AC;
    if (obj.type == S) {
        n = (x - obj.p).norm();
    }
    else if (obj.type == P) {
        n = (pn).norm();
    }
    else if (obj.type == T) {
        n = n0_t.norm();
    }
    Vec nl = n.dot(r.d) < 0 ? n : n * -1;
    int fanshe = rand() % 100;
    int fanshe2 = rand() % 100;
    int zheshe = rand() % 100;
    /*xΪ����,nΪ���巨����,nl��������������,������巨�����͹��߷��������ĵ��С����,���߱�Ϊ�෴����
    (��ʱ���ߴ��ڲ�����),(���ⲿ���Ĺ���,��������;���ڲ����Ĺ���,��������),fΪ������ɫ*/
    if (depth > 6) return Vec(); //���ݹ���ȴ���6,���غ�ɫ
    double p = f.x > f.y && f.x > f.z ? f.x : f.y > f.z ? f.y : f.z; /*��ȡRGB����ֵ������ֵ*/
    if (++depth > 5) { //�ݹ�ﵽ5ʱ�л��᷵��
        if (erand48(Xi) < p)
            f = f * (1 / p);
        else
            return obj.e;
    }
    if (fanshe <= obj.ref) {
        if (obj.diff >= fanshe2) { // ������(�ڰ������漴��һ������,Ȼ����еݹ�)
            double r1 = 2 * M_PI * erand48(Xi), r2 = erand48(Xi), r2s = sqrt(r2);
            //r1Ϊ���ѡȡ�ĽǶ�,��Χ�� 0 �� 2�� ֮��,r2�����ѡ����һ������(0-1),r2s�Ǿ��뿪���Ľ��
            Vec w = nl, u = ((fabs(w.x) > .1 ? Vec(0, 1, 0) : Vec(1, 0, 0)) % w).norm(), v = w % u;
            //fabs()�󸡵�������ֵ
            /*���ݷ��߹�����һ��������, w�뷨��ͬ��(��nά�ռ䵱��,��n����������(��ֱ)���������һ��������)
            ��w.x�ľ���ֵ>0.1��ʱ��,u��ֱ��(0,1,0)��w�ĵ�λ����,�����Ǵ�ֱ��(1,0,0)��w�ĵ�λ����
            ��������Ŀ���ǵ�w.x���ڻ�ӽ�0ʱ,���ܻ����������ص����*/
            Vec d = (u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2)).norm();//����ⷽ������
            return obj.e + f.mult(radiance(Ray(x, d), depth, Xi));
        }/*ȫ�ֹ��շ���,ʹ�����ؿ��޷������*/
        /*���淴��,
         ��ʽΪ ������߷�������-2*������*����ⷽ������dot������=������߷�������*/
        else if (obj.spec >= fanshe2) {
            Vec spec_t= r.d - n * 2 * n.dot(r.d);
            if (obj.type == S) {
                return obj.e + f.mult(radiance(Ray(x, spec_t), depth, Xi));
            }
            else if (obj.type==P){
                return obj.e + f.mult(radiance(Ray(x, spec_t), depth, Xi));
            }
        }
        //����Ϊ����
        Ray reflRay(x, r.d - n * 2 * n.dot(r.d));// �������
        bool into = n.dot(nl) > 0; /* ����Ĺ����Ƿ���������?���n��nlͬ������ߴ���߽���,������ߴ��ڲ�����*/
        double nc = ncg, nt = obj.refr_nt, nnt = into ? nc / nt : nt / nc, ddn = r.d.dot(nl), cos2t;/*ncΪ������������,ntΪ����������
        ��,nnt��ԭ���ʺ�Ŀ����ʵı�ֵ,ddn�ǹ��߷��������ͷ���nl�ļн�����ֵ,cos2t��cos(t)^2*/
        double r1 = 2 * M_PI * erand48(Xi), r2 = erand48(Xi), r2s = sqrt(r2);
        Vec w = nl, u = ((fabs(w.x) > .1 ? Vec(0, 1, 0) : Vec(1, 0, 0)) % w).norm(), v = w % u;
        Vec d = (u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2)).norm();
        if ((cos2t = 1 - nnt * nnt * (1 - ddn * ddn)) < 0)    /*ȫ����,�����ߴӽϸ� ������ �� ���� ���뵽�ϵ������ʵĽ���
            ʱ,�������Ǵ���ĳһ�ٽ�Ǧ�c(����Զ�� ���� )ʱ,����ǽ�����㹻��,�����Ĺ��߽������뿪����*/
            return obj.e + f.mult(radiance(reflRay, depth, Xi));
        Vec tdir = (r.d * nnt - n * ((into ? 1 : -1) * (ddn * nnt + sqrt(cos2t)))).norm();//������ߵķ���
        return obj.refr >= fanshe2 ? obj.e + f.mult(radiance(Ray(x, tdir), depth, Xi))+obj.i  :zheshe<=50?
            obj.e+radiance(reflRay, depth, Xi)* (obj.spec)*0.9 : obj.e + f.mult(radiance(Ray(x, d), depth, Xi)) * (obj.diff)*0.9;
    }
    else {
        return Vec();
    }
}
#define jz 0.01
Vec DeNoisy(int x, int y, int i, int w, int h, int samps, Vec c[], int i1=0) {
    Vec yansehuancun;
    double p = c[i].x > c[i].y && c[i].x > c[i].z ? c[i].x : c[i].y > c[i].z ? c[i].y : c[i].z;
    double a = 0;
    for (int j = 1; j <= i1; j++) {
        if (c[i].x <= jz && c[i].y <= jz && c[i].z <= jz && x > 0 && y > 0 && samps <= 100) {//����
            if (y > 0 + j && i - w - j <= w * h && i - w - j >= 0 && !(&(c[i - w - j]) == nullptr)) {
                yansehuancun = yansehuancun + c[i - w - j];
                a++;
            }
            if (y < h - 1 - j && i + w + j <= w * h && !(&(c[i + w + j]) == nullptr)) {
                yansehuancun = yansehuancun + c[i + w + j];
                a++;
            }
            if (x > 0 + j && i - 1 - j <= w * h && !(&(c[i - 1 - j]) == nullptr)) {
                yansehuancun = yansehuancun + c[i - 1 - j];
                a++;
            }
            if (x < w - (1 + j) && i + 1 + j <= w * h && !(&(c[i + 1 + j]) == nullptr)) {
                yansehuancun = yansehuancun + c[i + 1 + j];
                //a++;
            }
        }
    }
    if (yansehuancun.x > 0 && yansehuancun.y > 0 && yansehuancun.z > 0)
        return c[i] = yansehuancun * (((1 - p) / 2.5 + p) / a * 1.5);
}
Vec DeNoisy2(int x, int y, int i, int w, int h, int samps, Vec c[], int i1=0) {
    Vec yansehuancun;
    double p = c[i].x > c[i].y && c[i].x > c[i].z ? c[i].x : c[i].y > c[i].z ? c[i].y : c[i].z;
    double a = 0;
    for (int j = 1; j <= i1; j++) {
        if (c[i].x <= jz && c[i].y <= jz && c[i].z <= jz && x > 0 && y > 0 && samps <= 64) {//����
            if (y > 0 + j + 1 && i - w - j - 1 <= w * h && i - w - j >= 0 && !(&(c[i - w - j]) == nullptr)) {
                yansehuancun = yansehuancun + c[i - w - j - 1];
                a++;
            }
            if (y < h - 1 - j - 1 && i + w + j + 1 <= w * h && !(&(c[i + w + j]) == nullptr)) {
                yansehuancun = yansehuancun + c[i + w + j + 1];
                a++;
            }
            if (x > 0 + j + w && i - 1 - j - w <= w * h && !(&(c[i - 1 - j]) == nullptr)) {
                yansehuancun = yansehuancun + c[i - 1 - j - w];
                a++;
            }
            if (x < w - (1 + j - 1) && i + 1 + j + w <= w * h && !(&(c[i + 1 + j]) == nullptr)) {
                yansehuancun = yansehuancun + c[i + 1 + j + w];
                //a++;
            }
        }
    }
    if (yansehuancun.x > 0 && yansehuancun.y > 0 && yansehuancun.z > 0)
        return c[i] = yansehuancun * (((1 - p) / 2.5 + p) / a * 1.5);
}
bool js( Vec a,Vec b) {
    if (fabs(a.x - b.x) > 0.02 && fabs(a.y - b.x) > 0.02 && fabs(a.z - b.z) > 0.02) {
        return true;
    }
    else {
        return false;
    }
}
int main(int argc, char* argv[]) {
    //omp_set_num_threads(36);
    int w = 1024 / 2, h = 768 / 2, samps = argc == 2 ? atoi(argv[1]) / 4 : 100, samps2=samps; //ͼ���С����������
    Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm()); //�����λ�úͷ���
    /*cx��ʾÿ�κ����ƶ�ʱ�ƶ�����,cy��ʾ�����ƶ�ʱ�ƶ�����,rΪÿ�β���ʱ��¼�Ľ��,c��������������ͼ�������(��)*/
    Vec cx = Vec(w * .5135 / h), cy = (cx % cam.d).norm() * .5135, r, * c = new Vec[w * h];
	#pragma omp parallel for schedule(dynamic, 1) private(r) //���߳�
    for (int y = 0; y < h; y++) { // ѭ������ͼ���ÿһ��
        fprintf(stderr, "\r��Ⱦ��... %5.2f%%", 100. * y / (h - 1)); //�������
        for (unsigned short x = 0, Xi[3] = { 0,0,(unsigned short)(y * y * y) }; x < w; x++)
            for (int sy = 0, i = (h - y - 1) * w + x; sy < 2; sy++) //2x2��������
                for (int sx = 0; sx < 2; sx++, r = Vec()) { //2x2��������,r������¼���λ����ɫֵ
                    for (int s = 0; s < samps; s++) { //����
                        /*�˲���,�ò���λ�����м�ĸ��ʴ�,����Χ�ĸ���С(��)*/
                        double r1 = 2 * erand48(Xi), dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                        /*r1,r2��0-2����,��һ���������sqrt(r) - 1,һ������1 - sqrt(2 - r)*/
                        double r2 = 2 * erand48(Xi), dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                        //���ط������ߵķ���(��)
                        Vec d = cx * (((sx + .5 + dx) / 2 + x) / w - .5) + cy * (((sy + .5 + dy) / 2 + y) / h - .5) + cam.d;
                        /*
						��������Ĺ�����ǰ���Կ�ʼ���ڲ�, cam.o + d * 140���������(�������Ļ֮��ľ�����140����λ),
                        * (1. / samps)�������Ǽ�����չ��׵�ƽ��ֵ,��Ϊr = r + radiance(Ray(cam.o + d * 140, d.norm()), 0, Xi)�������
                        ���в���������ܺ�,����������ȿ��ܹ���
						*/
                        r = r + radiance(Ray(cam.o + d * 140, d.norm()), 0, Xi) * (1. / samps);
                        //x,y,����,��,��,��������,����ͼ�������,��������
                        DeNoisy(x, y, i, w, h, samps, c, 1);
                        //x,y,����,��,��,��������,����ͼ�������,��������
                        DeNoisy2(x, y, i, w, h, samps, c, 1);
                    }
                    c[i] = c[i] + Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * .25; //��Ϊ��2*2��������,ÿ�����ֻռ1/4,���Գ�0.25
                    if (c[i - 1].x > 0.95 && c[i - 1].y > 0.95 && c[i - 1].z > 0.95 || c[i - 1].x < 0.05 && c[i - 1].y < 0.05 && c[i - 1].z < 0.05) { //��Ҫ�Բ���
                        samps = 1;
                        //c[i - 1] = Vec(0, 0, 1); //Debug
                    }
                    else {
                        samps = samps2;
                    }
                }
    }

    FILE* f = fopen("image.ppm", "w"); //�ļ�д��
    fprintf(f, "P3\n%d %d\n%d\n", w, h, 255);
    for (int i = 0; i < w * h; i++)
        fprintf(f, "%d %d %d ", toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
    system("pause");
}
