//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                          INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
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
// Ficheiro: vc.c
//
// Descrição:
//      Ficheiro que contém a implementação das funções da biblioteca de
//      Visão por Computador utilizadas no projeto.
//
//      Inclui funções para criar e libertar imagens, converter imagens entre
//      diferentes espaços de cor, realizar segmentação em HSV, aplicar
//      operações morfológicas, efetuar a etiquetagem de componentes ligados
//      e calcular informações relativas aos objetos detetados.
//
//      As funções implementadas neste ficheiro são chamadas pelo programa
//      principal para processar cada frame do vídeo e identificar as
//      laranjas presentes na imagem, classificá-las e categorizá-las.
//
// Data: julho de 2026
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "vc.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define TOTAL_CALIBERS 14

typedef struct {
	int caliber;
	int min_diameter;
	int max_diameter;
} CaliberMap;

static const CaliberMap orange_calibers[TOTAL_CALIBERS] = {
	{0,  100, 999}, // 100 e +
	{1,   87, 100},
	{2,   84,  96},
	{3,   81,  92},
	{4,   77,  88},
	{5,   73,  84},
	{6,   70,  80},
	{7,   67,  76},
	{8,   64,  73},
	{9,   62,  70},
	{10,  60,  68},
	{11,  58,  66},
	{12,  56,  63},
	{13,  53,  60}
};

IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL)
		return NULL;
	if ((levels <= 0) || (levels > 255))
		return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}

IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}

int vc_bgr_to_hsv(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int byterperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf, max, min, h, s, v;

	// verificacao de erros
	if ((src->width <= 0) || (src->height <= 0) || src->data == NULL || dst->data == NULL)
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 3) || (dst->channels != 3))
		return 0;

	// converter a imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline + x * channels;//calcular a posicao do pixel no array linear
			pos_dst = y * byterperline_dst + x * channels_dst;

			//normalizacao entre 0 e 1
			bf = (float)datasrc[pos_src] / 255.0f;
			gf = (float)datasrc[pos_src + 1] / 255.0f;
			rf = (float)datasrc[pos_src + 2] / 255.0f;

			max = rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf);
			min = rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf);

			v = max;

			if (max == 0)
				s = 0;
			else
				s = (max - min) / max;

			if (s == 0)
				h = 0;
			else if (max == rf)
				h = 60.0f * ((gf - bf) / (max - min));
			else if (max == gf)
				h = 60.0f * ((bf - rf) / (max - min)) + 120.0f;
			else
				h = 60.0f * ((rf - gf) / (max - min)) + 240.0f;

			if (h < 0)
				h += 360.0f;
			//converte H da escala de 0-360 para 0-255 e restantes de 0-1 para 0-255
			datadst[pos_dst] = (unsigned char)(h / 360.0f * 255.0f);
			datadst[pos_dst + 1] = (unsigned char)(s * 255.0f);
			datadst[pos_dst + 2] = (unsigned char)(v * 255.0f);
		}
	}
	return 1;
}

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int byterperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	unsigned char h, s, v;

	// verificacao de erros
	if ((src->width <= 0) || (src->height <= 0) || src->data == NULL || dst->data == NULL)
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 3) || (dst->channels != 1))
		return 0;

	// ------------------------------------------------------------------
	// Converte os limites (0-360 e 0-100) para a escala real da imagem (0-255)
	// ------------------------------------------------------------------
	int hmin_255 = (int)(((float)hmin / 360.0f) * 255.0f);
	int hmax_255 = (int)(((float)hmax / 360.0f) * 255.0f);
	int smin_255 = (int)(((float)smin / 100.0f) * 255.0f);
	int smax_255 = (int)(((float)smax / 100.0f) * 255.0f);
	int vmin_255 = (int)(((float)vmin / 100.0f) * 255.0f);
	int vmax_255 = (int)(((float)vmax / 100.0f) * 255.0f);

	// segmentar a imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * byterperline_dst + x * channels_dst;

			h = datasrc[pos_src];
			s = datasrc[pos_src + 1];
			v = datasrc[pos_src + 2];

			// Comparar com as variaveis ja convertidas para 255!
			if ((h >= hmin_255) && (h <= hmax_255) && (s >= smin_255) && (s <= smax_255) && (v >= vmin_255) && (v <= vmax_255))
				datadst[pos_dst] = 255; // Branco
			else
				datadst[pos_dst] = 0; // Preto
		}
	}
	return 1;
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc;
	unsigned char* datadst;
	int bytesperline, channels;
	int byterperline_dst, channels_dst;
	int width, height;
	int x, y, kx, ky;
	long int pos_dst;

	// verificacao de erros
	if (src == NULL || dst == NULL)
		return 0;

	if ((src->width <= 0) || (src->height <= 0) || src->data == NULL || dst->data == NULL)
		return 0;

	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;

	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	if (kernel <= 0 || kernel % 2 == 0)
		return 0;

	datasrc = (unsigned char*)src->data;
	datadst = (unsigned char*)dst->data;

	bytesperline = src->width * src->channels;
	channels = src->channels;

	byterperline_dst = dst->width * dst->channels;
	channels_dst = dst->channels;

	width = src->width;
	height = src->height;

	// converter a imagem usando o metodo de dilatacao
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_dst = y * byterperline_dst + x * channels_dst;

			bool found = false;

			// Iterar sobre o kernel
			for (ky = -kernel / 2; ky <= kernel / 2; ky++)
			{
				for (kx = -kernel / 2; kx <= kernel / 2; kx++)
				{
					int neighbor_x = x + kx;
					int neighbor_y = y + ky;

					// Verificar se o vizinho esta dentro dos limites da imagem
					if (neighbor_x >= 0 && neighbor_x < width &&
						neighbor_y >= 0 && neighbor_y < height)
					{
						long int neighbor_pos = neighbor_y * bytesperline + neighbor_x * channels;

						if (datasrc[neighbor_pos] == 255)
						{
							found = true;
							break;
						}
					}
				}

				if (found)
					break;
			}

			datadst[pos_dst] = found ? 255 : 0;
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc;
	unsigned char* datadst;
	int bytesperline, channels;
	int bytesperline_dst, channels_dst;
	int width, height;
	int x, y, kx, ky;
	long int pos_dst;

	// verificacao de erros
	if (src == NULL || dst == NULL)
		return 0;

	if ((src->width <= 0) || (src->height <= 0) || src->data == NULL || dst->data == NULL)
		return 0;

	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;

	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	if (kernel <= 0 || kernel % 2 == 0)
		return 0;

	datasrc = (unsigned char*)src->data;
	datadst = (unsigned char*)dst->data;

	width = src->width;
	height = src->height;

	channels = src->channels;
	channels_dst = dst->channels;

	bytesperline = src->width * src->channels;
	bytesperline_dst = dst->width * dst->channels;

	// converter a imagem usando o metodo de erosao
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_dst = y * bytesperline_dst + x * channels_dst;

			bool should_erode = false;

			// Iterar sobre o kernel
			for (ky = -kernel / 2; ky <= kernel / 2; ky++)
			{
				for (kx = -kernel / 2; kx <= kernel / 2; kx++)
				{
					int neighbor_x = x + kx;
					int neighbor_y = y + ky;

					// Verificar se o vizinho esta dentro dos limites da imagem
					if (neighbor_x >= 0 && neighbor_x < width &&
						neighbor_y >= 0 && neighbor_y < height)
					{
						long int neighbor_pos = neighbor_y * bytesperline + neighbor_x * channels;

						// Se algum vizinho for preto, o pixel atual deve ser erodido
						if (datasrc[neighbor_pos] == 0)
						{
							should_erode = true;
							break;
						}
					}
				}

				if (should_erode)
					break;
			}

			datadst[pos_dst] = should_erode ? 0 : 255;
		}
	}

	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernel_erode, int kernel_dilate)
{
	IVC* temp = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (temp == NULL)
		return 0;

	if (!vc_binary_erode(src, temp, kernel_erode))
	{
		vc_image_free(temp);
		return 0;
	}

	if (!vc_binary_dilate(temp, dst, kernel_dilate))
	{
		vc_image_free(temp);
		return 0;
	}

	vc_image_free(temp);
	return 1;
}

int vc_binary_close(IVC* src, IVC* dst, int kernel_dilate, int kernel_erode)
{
	IVC* temp = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (temp == NULL)
		return 0;

	if (!vc_binary_dilate(src, temp, kernel_dilate))
	{
		vc_image_free(temp);
		return 0;
	}

	if (!vc_binary_erode(temp, dst, kernel_erode))
	{
		vc_image_free(temp);
		return 0;
	}

	vc_image_free(temp);
	return 1;
}

int vc_blob_is_cut(OVC blob, int image_width, int image_height, int margin)
{
	if (blob.x <= margin)
		return 1;

	if (blob.y <= margin)
		return 1;

	if (blob.x + blob.width >= image_width - margin)
		return 1;

	if (blob.y + blob.height >= image_height - margin)
		return 1;

	return 0;
}

OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc, * datadst;
	int width, height, bytesperline, channels;
	int x, y, i, a, b;
	long int pos, size;
	long int posA, posB, posC, posD, posX;
	int labeltable[256] = { 0 };
	int final_labels[256] = { 0 };//utilizado para contar as labels sem repeticao
	int label = 1;
	int vizinhos[4];
	int menor_label;
	int etiqueta_antiga;
	OVC* blobs;

	// Verificacao de erros
	if ((src == NULL) || (dst == NULL) || (nlabels == NULL))
		return NULL;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL) || (dst->data == NULL))
		return NULL;

	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return NULL;

	if (src->channels != 1)
		return NULL;

	datasrc = (unsigned char*)src->data;
	datadst = (unsigned char*)dst->data;

	width = src->width;
	height = src->height;
	bytesperline = src->bytesperline;
	channels = src->channels;

	*nlabels = 0;

	// Copiar imagem binaria para a imagem de etiquetas
	memcpy(datadst, datasrc, bytesperline * height);

	// Garantir que a imagem fica mesmo binaria:
	// fundo = 0, objeto = 255
	size = bytesperline * height;

	for (i = 0; i < size; i++)
	{
		if (datadst[i] != 0)
			datadst[i] = 255;
	}

	// Limpar os rebordos da imagem, previne problemas nas bordas, previne aceder a espaços de memoria indevidos
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}

	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Primeira passagem: atribuir etiquetas provisorias
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;

			// So processa pi­xeis brancos ainda nao etiquetados
			if (datadst[posX] == 255)
			{
				vizinhos[0] = datadst[posA];
				vizinhos[1] = datadst[posB];
				vizinhos[2] = datadst[posC];
				vizinhos[3] = datadst[posD];

				menor_label = 255;

				// Procurar a menor etiqueta entre os vizinhos A, B, C e D
				for (a = 0; a < 4; a++)
				{
					if (vizinhos[a] != 0)
					{
						if (labeltable[vizinhos[a]] < menor_label)
							menor_label = labeltable[vizinhos[a]];
					}
				}

				// Caso 1: nenhum vizinho tem etiqueta
				// Entao comeca um novo blob
				if (menor_label == 255)
				{
					if (label >= 255)
						return NULL;

					datadst[posX] = (unsigned char)label;//inicializado em cima a 1, primeira etiqueta encontrada
					labeltable[label] = label;
					label++;
				}
				// Caso 2: pelo menos um vizinho ja tem etiqueta
				else
				{
					datadst[posX] = (unsigned char)menor_label;

					// Atualizar equivalencias:
					// se algum vizinho tinha outra etiqueta, ela passa a equivaler a menor etiqueta
					for (a = 0; a < 4; a++)
					{
						if (vizinhos[a] != 0)
						{
							etiqueta_antiga = labeltable[vizinhos[a]];

							if (etiqueta_antiga != menor_label)
							{
								for (b = 1; b < label; b++)
								{
									if (labeltable[b] == etiqueta_antiga)
										labeltable[b] = menor_label;
								}
							}
						}
					}
				}
			}
		}
	}

	// Segunda passagem: corrigir etiquetas finais na imagem de acordo com a labeltable
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			pos = y * bytesperline + x * channels;

			if (datadst[pos] != 0)
				datadst[pos] = (unsigned char)labeltable[datadst[pos]];//troca pelo valor da labeltable, ex labletable[4]=2 nesta passagem troca o valor da etiqueta que tinha por 2
		}
	}

	// Contar etiquetas finais sem repeticao
	*nlabels = 0;//nlabels do main passa a valer 0, apontador para nlabels da main

	for (a = 1; a < label; a++)
	{
		int etiqueta_final = labeltable[a];//qual a etiqueta final da etiqueta provisoria a

		if ((etiqueta_final != 0) && (final_labels[etiqueta_final] == 0))//ou seja a etiqueta ainda nao foi contada
		{
			final_labels[etiqueta_final] = 1;//marca como contada, ja entrou na lista de blobs finais
			labeltable[*nlabels] = etiqueta_final;
			(*nlabels)++;
		}
	}

	// Se nao encontrou blobs
	if (*nlabels == 0)
		return NULL;

	// Criar vetor de blobs
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));

	if (blobs == NULL)
		return NULL;

	// Guardar a etiqueta de cada blob
	for (a = 0; a < *nlabels; a++)
	{
		blobs[a].label = labeltable[a];
	}

	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificacao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Conta a area de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Area
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x)
						xmin = x;
					if (ymin > y)
						ymin = y;
					if (xmax < x)
						xmax = x;
					if (ymax < y)
						ymax = y;

					// Perimetro
					// Se pelo menos um dos quatro vizinhos nao pertence ao mesmo label, entao trata-se de um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Calculo do Centro de Gravidade
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

float vc_blob_circularity(OVC blob)
{
	if (blob.perimeter <= 0)
		return 0.0f;

	return (4.0f * 3.14159f * (float)blob.area) /
		((float)blob.perimeter * (float)blob.perimeter);
}

float vc_blob_shape_ratio(OVC blob)
{
	int menor;
	int maior;

	if (blob.width <= 0 || blob.height <= 0)
		return 0.0f;

	if (blob.width < blob.height)
	{
		menor = blob.width;
		maior = blob.height;
	}
	else
	{
		menor = blob.height;
		maior = blob.width;
	}

	return (float)menor / (float)maior;
}

float vc_blob_shape_defect(OVC blob)
{
	float ratio = vc_blob_shape_ratio(blob);

	if (ratio <= 0.0f)
		return 100.0f;

	return (1.0f - ratio) * 100.0f;
}

int conversion_pixels_to_mm(int pixel_value, int reference_pixels, int reference_mms, float* converted_value)
{
	float calc = ((float)pixel_value / ((float)reference_pixels) * (float)reference_mms);

	*converted_value = calc;

	return 1;
}

int get_caliber(int diameter_mm) {

	if (diameter_mm < 53) return -1;


	for (int i = 0; i < TOTAL_CALIBERS; i++) {

		if (diameter_mm >= orange_calibers[i].min_diameter &&
			diameter_mm <= orange_calibers[i].max_diameter) {

			return orange_calibers[i].caliber;
		}
	}

	return -1;
}

int vc_orange_colour_defect_percentage_by_segmentation( IVC* labels, IVC* good_colour_mask,	OVC blob, float* defect_percentage){
	unsigned char* data_labels;
	unsigned char* data_good;
	int x, y;
	int total_pixels = 0;
	int defect_pixels = 0;

	if (labels == NULL || good_colour_mask == NULL || defect_percentage == NULL)
		return 0;

	if (labels->data == NULL || good_colour_mask->data == NULL)
		return 0;

	if (labels->channels != 1 || good_colour_mask->channels != 1)
		return 0;

	if (labels->width != good_colour_mask->width || labels->height != good_colour_mask->height)
		return 0;

	data_labels = labels->data;
	data_good = good_colour_mask->data;

	for (y = blob.y; y < blob.y + blob.height; y++)//percorre toda a bounding box
	{
		for (x = blob.x; x < blob.x + blob.width; x++)
		{
			if (x < 0 || y < 0 || x >= labels->width || y >= labels->height)
				continue;

			long int pos_labels = y * labels->bytesperline + x * labels->channels;
			long int pos_good = y * good_colour_mask->bytesperline + x * good_colour_mask->channels;

			// Só analisa pixels que pertencem mesmo a esta laranja
			if (data_labels[pos_labels] == blob.label)
			{
				total_pixels++;

				// Se pertence à laranja, mas não tem cor "boa", conta como defeito
				if (data_good[pos_good] == 0)
				{
					defect_pixels++;
				}
			}
		}
	}

	if (total_pixels == 0)
	{
		*defect_percentage = 100.0f;
		return 1;
	}

	*defect_percentage = ((float)defect_pixels / (float)total_pixels) * 100.0f;

	return 1;
}

	
static void draw_pixel_bgr(unsigned char* data, int width, int height, int channels,
	int x, int y, unsigned char b, unsigned char g, unsigned char r)
{
	int pos;

	if (data == NULL || channels < 3 ||
		x < 0 || y < 0 || x >= width || y >= height)
	{
		return;
	}

	pos = (y * width + x) * channels;
	data[pos] = b;
	data[pos + 1] = g;
	data[pos + 2] = r;
}

void draw_cross_bgr(unsigned char* data, int width, int height, int channels,
	int cx, int cy, int size, unsigned char b, unsigned char g, unsigned char r)
{
	int i;

	for (i = -size; i <= size; i++)
	{
		draw_pixel_bgr(data, width, height, channels, cx + i, cy, b, g, r);
		draw_pixel_bgr(data, width, height, channels, cx, cy + i, b, g, r);
	}
}

void draw_rectangle_bgr(unsigned char* data, int width, int height, int channels,
	int x, int y, int w, int h, int thickness,
	unsigned char b, unsigned char g, unsigned char r)
{
	int t, i, j;

	if (w <= 0 || h <= 0 || thickness <= 0)
	{
		return;
	}

	for (t = 0; t < thickness; t++)
	{
		for (i = x; i < x + w; i++)
		{
			draw_pixel_bgr(data, width, height, channels, i, y + t, b, g, r);
			draw_pixel_bgr(data, width, height, channels, i, y + h - 1 - t, b, g, r);
		}

		for (j = y; j < y + h; j++)
		{
			draw_pixel_bgr(data, width, height, channels, x + t, j, b, g, r);
			draw_pixel_bgr(data, width, height, channels, x + w - 1 - t, j, b, g, r);
		}
	}
}

const char* classify_orange_quality(int caliber, float circularity,	float shape_defect,	float color_defect,	int is_cut)
{
	if (is_cut)
		return "Nao classificada";

	if (caliber == -1)
		return "Fora calibre";

	/*
		shape_defect:
		0% significa largura e altura muito parecidas.
		Quanto maior, mais deformada é a laranja.

		color_defect:
		percentagem de pixels dentro da laranja que não parecem ter cor normal.
	*/

	if (circularity >= 0.85f && shape_defect <= 5.0f && color_defect <= 4.0f)
		return "Extra";

	if (circularity >= 0.75f && shape_defect <= 12.0f && color_defect <= 10.0f)
		return "I";

	if (circularity >= 0.60f && shape_defect <= 25.0f && color_defect <= 25.0f)
		return "II";

	return "Rejeitada";
}