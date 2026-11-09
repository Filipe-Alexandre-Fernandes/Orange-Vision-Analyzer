//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                         INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                              ESCOLA SUPERIOR DE TECNOLOGIA
//                     LICENCIATURA EM ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                                  VISÃO POR COMPUTADOR
//
//                                 TRABALHO PRÁTICO
//                         DETEÇÃO E CLASSIFICAÇÃO DE LARANJAS
//
// Docente: Duarte Duque
// 
// Grupo:
//      Ana Rita Ranito - a31511
//      Filipe Fernandes - a31498
//
// Ficheiro: vc.h
//
// Descrição:
//      Ficheiro de cabeçalho da biblioteca de Visão por Computador.
//
//      Contém a definição das estruturas utilizadas para representar imagens
//      e objetos detetados, nomeadamente as estruturas IVC e OVC. Inclui ainda
//      os protótipos das funções implementadas no ficheiro vc.c, permitindo
//      que estas sejam utilizadas pelo programa principal.
//
//      Este ficheiro estabelece a ligação entre o ficheiro Laranjas.cpp e a
//      biblioteca de processamento de imagem desenvolvida para o projeto.
//
// Data: julho de 2026
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef VC_H
#define VC_H

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Estrutura de uma imagem
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef struct
{
	unsigned char* data;
	int width, height;
	int channels;     
	int levels;      
	int bytesperline;  
} IVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Estrutura de um blob/objeto
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef struct
{
	int x, y, width, height;
	int area;
	int xc, yc;
	int perimeter;
	int label;
} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Funções usadas pelo Laranjas.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);
int vc_bgr_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax,
int smin, int smax, int vmin, int vmax);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_open(IVC* src, IVC* dst, int kernel_erode, int kernel_dilate);
int vc_binary_close(IVC* src, IVC* dst, int kernel_dilate, int kernel_erode);
int vc_blob_is_cut(OVC blob, int image_width, int image_height, int margin);
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);
int conversion_pixels_to_mm(int pixel_value, int reference_pixels, int reference_mms, float* converted_value);
int get_caliber(int diameter_mm);
float vc_blob_circularity(OVC blob);
float vc_blob_shape_ratio(OVC blob);
float vc_blob_shape_defect(OVC blob);
int vc_orange_colour_defect_percentage_by_segmentation(IVC* labels,	IVC* good_colour_mask,	OVC blob, float* defect_percentage);
const char* classify_orange_quality(int caliber, float circularity, float shape_defect,	float color_defect,	int is_cut);
void draw_cross_bgr(unsigned char* data, int width, int height, int channels, int cx, int cy, int size,	unsigned char b, unsigned char g, unsigned char r);
void draw_rectangle_bgr(unsigned char* data, int width, int height, int channels, int x, int y, int w, int h, int thickness, unsigned char b, unsigned char g, unsigned char r);

#endif
