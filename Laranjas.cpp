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
// 
// Ficheiro: Laranjas.cpp
//
// Descrição:
//      Ficheiro principal da aplicação. É responsável pela leitura e
//      processamento do vídeo, pela conversão das imagens para o formato
//      utilizado pela biblioteca própria e pela aplicação das diferentes
//      etapas do processamento de imagem.
//
//      Neste ficheiro é realizada a segmentação das laranjas, a aplicação
//      das operações morfológicas, a identificação dos objetos presentes
//      na imagem, o cálculo das suas características e o rastreamento das
//      laranjas ao longo dos vários frames do vídeo.
//
//
// Data: julho de 2026
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdlib>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

extern "C" {
#include "vc.h"
}

// ------------------------------------------------------------------
// ESTRUTURA DE MEMORIA PARA O RASTREAMENTO
// ------------------------------------------------------------------
struct Ponto {
	int x;
	int y;
};

struct LaranjaRastreada {
	Ponto centro;
	int frames_ausente;
};

int main(void) {
	constexpr auto SCALEPIXELS = 280; // pixeis
	constexpr auto SCALEMM = 55;   // mm
	constexpr int KERNEL_MORFOLOGIA = 3;

	// Segmentacao principal: mais tolerante, serve para encontrar a laranja
	constexpr int H_MIN_DETECAO = 10;
	constexpr int H_MAX_DETECAO = 35;
	constexpr int S_MIN_DETECAO = 45;
	constexpr int V_MIN_DETECAO = 18;

	// Segmentacao da cor boa: mais apertada, serve para avaliar defeitos de cor
	constexpr int H_MIN_COR_BOA = 13;
	constexpr int H_MAX_COR_BOA = 32;
	constexpr int S_MIN_COR_BOA = 53;
	constexpr int V_MIN_COR_BOA = 20;

	// Filtros dos blobs
	constexpr int AREA_MINIMA = 2000;
	constexpr float CIRCULARIDADE_MINIMA = 0.50f;
	constexpr int SQUARISH_MIN = 70;

	constexpr int MARGEM_CORTE = 15;
	constexpr int TOLERANCIA_MOVIMENTO = 70;
	constexpr int MAX_FRAMES_MEMORIA = 15;

	char videofile[10] = "video.avi";
	cv::VideoCapture capture;

	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int currentframe;
	} video;

	std::string str;
	int key = 0;

	capture.open(videofile);

	if (!capture.isOpened()) {
		std::cerr << "Erro ao abrir o ficheiro de video!\n";
		return 1;
	}

	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);

	cv::namedWindow("DETECAO DE LARANJAS", cv::WINDOW_AUTOSIZE);
	cv::namedWindow("MASCARA BINARIA", cv::WINDOW_AUTOSIZE);

	cv::Mat frame;

	// ------------------------------------------------------------------
	// VARIAVEIS DE RASTREAMENTO E ESTATISTICAS
	// ------------------------------------------------------------------
	int total_laranjas = 0;
	int global_min_diameter = 9999;
	int global_max_diameter = 0;

	std::vector<LaranjaRastreada> memoria_rastreio;
	std::vector<Ponto> centros_atuais;
	std::vector<LaranjaRastreada> memoria_atualizada;

	memoria_rastreio.reserve(50);
	centros_atuais.reserve(50);
	memoria_atualizada.reserve(50);


	// ------------------------------------------------------------------
	// CRIACAO DAS IMAGENS AUXILIARES
	// ------------------------------------------------------------------
	IVC* image_src = vc_image_new(video.width, video.height, 3, 255);
	IVC* image_hsv = vc_image_new(video.width, video.height, 3, 255);
	IVC* image_mask = vc_image_new(video.width, video.height, 1, 255);
	IVC* image_temp1 = vc_image_new(video.width, video.height, 1, 255);
	IVC* image_temp2 = vc_image_new(video.width, video.height, 1, 255);
	IVC* image_labels = vc_image_new(video.width, video.height, 1, 255);
	IVC* image_good_colour = vc_image_new(video.width, video.height, 1, 255);

	if (image_src == NULL || image_hsv == NULL || image_mask == NULL ||
		image_temp1 == NULL || image_temp2 == NULL || image_labels == NULL ||
		image_good_colour == NULL)
	{
		std::cerr << "Erro ao criar imagens auxiliares!\n";

		vc_image_free(image_src);
		vc_image_free(image_hsv);
		vc_image_free(image_mask);
		vc_image_free(image_temp1);
		vc_image_free(image_temp2);
		vc_image_free(image_labels);
		vc_image_free(image_good_colour);

		capture.release();
		cv::destroyAllWindows();

		return 1;
	}

	while (key != 'q') {
		capture.read(frame);
		if (frame.empty()) break;

		memcpy(image_src->data, frame.data, video.width * video.height * 3);
		video.currentframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		centros_atuais.clear();//de modo a obter so as informações da frame e nao trazer das anteriores, a memoria é memoria_rastreio
		memoria_atualizada.clear();

		// ------------------------------------------------------------------
		// PROCESSAMENTO DA FRAME
		// ------------------------------------------------------------------
		vc_bgr_to_hsv(image_src, image_hsv);

		// Mascara principal: detecao da laranja
		vc_hsv_segmentation(image_hsv, image_mask, H_MIN_DETECAO, H_MAX_DETECAO, S_MIN_DETECAO, 100, V_MIN_DETECAO, 100);

		// Mascara mais apertada: cor considerada boa
		vc_hsv_segmentation(image_hsv, image_good_colour, H_MIN_COR_BOA, H_MAX_COR_BOA, S_MIN_COR_BOA, 100, V_MIN_COR_BOA, 100);

		// Limpeza da mascara principal
		// ----------------------------------------------------------
		// ABERTURA: erosao seguida de dilatacao
		// image_mask -> image_temp1 -> image_temp2
		// ----------------------------------------------------------

		// ----------------------------------------------------------
		// FECHO: dilatacao seguida de erosao
		// image_temp2 -> image_temp1 -> image_temp2
		// ----------------------------------------------------------

		bool morfologia_ok = true;

		if (vc_binary_erode(image_mask, image_temp1, KERNEL_MORFOLOGIA) == 0){
			morfologia_ok = false;
		}
		else if (vc_binary_dilate(image_temp1, image_temp2,	KERNEL_MORFOLOGIA) == 0){
			morfologia_ok = false;
		}
		
		else if (vc_binary_dilate(image_temp2, image_temp1,	KERNEL_MORFOLOGIA) == 0){
			morfologia_ok = false;
		}
		else if (vc_binary_erode(image_temp1, image_temp2, KERNEL_MORFOLOGIA) == 0){
			morfologia_ok = false;
		}
		//buffer ping-pong

		if (!morfologia_ok)
		{
			std::cerr << "Erro nas operacoes morfologicas!\n";
			break;
		}
		
		// ------------------------------------------------------------------
		// ANALISE DE BLOBS
		// ------------------------------------------------------------------
		int nblobs = 0;
		int real_blobs = 0;

		OVC* blobs = vc_binary_blob_labelling(image_temp2, image_labels, &nblobs);

		if (blobs != NULL) {
			vc_binary_blob_info(image_labels, blobs, nblobs);

			for (int i = 0; i < nblobs; i++) {

				float circularity = vc_blob_circularity(blobs[i]);
				float squarish = vc_blob_shape_ratio(blobs[i]);
				int squarish_percentage = (int)(squarish * 100.0f);
				//filtros para aceitar blob como laranja
				if (blobs[i].area > AREA_MINIMA && circularity >= CIRCULARIDADE_MINIMA && squarish_percentage >= SQUARISH_MIN) {
					real_blobs++;

					int centro_x = blobs[i].xc;
					int centro_y = blobs[i].yc;

					centros_atuais.push_back({ centro_x, centro_y });

					draw_cross_bgr(frame.data, video.width, video.height, 3, centro_x, centro_y, 5, 0, 0, 255);

					draw_rectangle_bgr(frame.data, video.width, video.height, 3, blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height, 3, 0, 165, 255);

					// ----------------------------------------------------------
					// CALCULO DAS MEDIDAS
					// ----------------------------------------------------------
					float scale_diameter = 0.0f;
					float scale_area = 0.0f;
					float scale_perimeter = 0.0f;

					int diameter_pixels = (blobs[i].width + blobs[i].height) / 2;

					conversion_pixels_to_mm(diameter_pixels, SCALEPIXELS, SCALEMM, &scale_diameter);
					conversion_pixels_to_mm(blobs[i].area, SCALEPIXELS * SCALEPIXELS, SCALEMM * SCALEMM, &scale_area);
					conversion_pixels_to_mm(blobs[i].perimeter, SCALEPIXELS, SCALEMM, &scale_perimeter);

					int scale_diameter_rounded = (int)std::round(scale_diameter);
					int scale_area_rounded = (int)std::round(scale_area);
					int scale_perimeter_rounded = (int)std::round(scale_perimeter);

					int caliber_diameter = get_caliber(scale_diameter_rounded);

					float shape_defect = vc_blob_shape_defect(blobs[i]);
					int shape_defect_rounded = (int)std::round(shape_defect);

					bool laranja_cortada = vc_blob_is_cut( blobs[i], video.width, video.height, MARGEM_CORTE);

					float color_defect = 0.0f;

					vc_orange_colour_defect_percentage_by_segmentation(image_labels, image_good_colour,	blobs[i], &color_defect);

					int color_defect_rounded = (int)std::round(color_defect);

					const char* category = classify_orange_quality(caliber_diameter, circularity, shape_defect, color_defect, laranja_cortada ? 1 : 0);

					// Estatisticas globais apenas se a laranja nao estiver cortada
					if (!laranja_cortada) {
						if (scale_diameter_rounded > global_max_diameter)
							global_max_diameter = scale_diameter_rounded;

						if (scale_diameter_rounded < global_min_diameter)
							global_min_diameter = scale_diameter_rounded;
					}

					// ----------------------------------------------------------
					// TEXTO SOBRE CADA LARANJA
					// ----------------------------------------------------------
					std::string label_circ = "Laranja";
					std::string label_category = "Categoria: " + std::string(category);
					std::string label_shape_defect = "Def. forma: " + std::to_string(shape_defect_rounded) + "%";
					std::string label_color_defect = "Def. cor/casca: " + std::to_string(color_defect_rounded) + "%";
					std::string label_calibre = "Calibre: " + std::to_string(caliber_diameter);
					std::string label_diameter = "Diametro: " + std::to_string(scale_diameter_rounded) + "mm";
					std::string label_area = "Area: " + std::to_string(scale_area_rounded) + "mm2";
					std::string label_perimeter = "Perimetro: " + std::to_string(scale_perimeter_rounded) + "mm";

					cv::putText(frame, label_circ, cv::Point(blobs[i].x, blobs[i].y - 145), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(124, 0, 29), 2);
					cv::putText(frame, label_category, cv::Point(blobs[i].x, blobs[i].y - 125), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(124, 0, 29), 2);
					cv::putText(frame, label_shape_defect, cv::Point(blobs[i].x, blobs[i].y - 105), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(124, 0, 29), 2);
					cv::putText(frame, label_color_defect, cv::Point(blobs[i].x, blobs[i].y - 85), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(124, 0, 29), 2);
					cv::putText(frame, label_calibre, cv::Point(blobs[i].x, blobs[i].y - 65), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(124, 0, 29), 2);
					cv::putText(frame, label_diameter, cv::Point(blobs[i].x, blobs[i].y - 45), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(124, 0, 29), 2);
					cv::putText(frame, label_area, cv::Point(blobs[i].x, blobs[i].y - 25), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(124, 0, 29), 2);
					cv::putText(frame, label_perimeter, cv::Point(blobs[i].x, blobs[i].y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(124, 0, 29), 2);
				}
			}

			free(blobs);
		}

//------------------------------------------------------------------
// LOGICA DE RASTREAMENTO COM MEMORIA
//------------------------------------------------------------------

		for (size_t a = 0; a < centros_atuais.size(); a++) {
			bool nova_laranja = true;

			for (size_t b = 0; b < memoria_rastreio.size(); b++) {

				if (memoria_rastreio[b].frames_ausente == 9999)
					continue;

				int dx = centros_atuais[a].x - memoria_rastreio[b].centro.x;
				int dy = centros_atuais[a].y - memoria_rastreio[b].centro.y;

				float dist = std::sqrt((float)(dx * dx + dy * dy));//raiz quadrada de (x2-x1)^2+(y2-y1)^2

				if (dist < TOLERANCIA_MOVIMENTO) {
					nova_laranja = false;
					memoria_atualizada.push_back({ centros_atuais[a], 0 });
					memoria_rastreio[b].frames_ausente = 9999;
					break;
				}
			}

			if (nova_laranja) {
				total_laranjas++;
				memoria_atualizada.push_back({ centros_atuais[a], 0 });
			}
		}

		for (size_t b = 0; b < memoria_rastreio.size(); b++) {
			if (memoria_rastreio[b].frames_ausente < MAX_FRAMES_MEMORIA) {
				memoria_rastreio[b].frames_ausente++;
				memoria_atualizada.push_back(memoria_rastreio[b]);
			}
		}

		memoria_rastreio = memoria_atualizada;

		// ------------------------------------------------------------------
		// INFORMACAO GLOBAL NO VIDEO
		// ------------------------------------------------------------------
		str = std::string("Laranjas Visiveis: ").append(std::to_string(real_blobs));
		cv::putText(frame, str, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 3);
		cv::putText(frame, str, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(124, 0, 29), 2);

		std::string str_total = std::string("Total: ").append(std::to_string(total_laranjas));
		cv::putText(frame, str_total, cv::Point(20, 70), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 3);
		cv::putText(frame, str_total, cv::Point(20, 70), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(124, 0, 29), 2);

		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 110), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 3);
		cv::putText(frame, str, cv::Point(20, 110), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(124, 0, 29), 2);

		str = std::string("Total Frames: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 140), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 3);
		cv::putText(frame, str, cv::Point(20, 140), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(124, 0, 29), 2);

		str = std::string("FPS: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 170), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 3);
		cv::putText(frame, str, cv::Point(20, 170), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(124, 0, 29), 2);

		str = std::string("N. DA FRAME: ").append(std::to_string(video.currentframe));
		cv::putText(frame, str, cv::Point(20, 200), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 3);
		cv::putText(frame, str, cv::Point(20, 200), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(124, 0, 29), 2);

		// ------------------------------------------------------------------
		// HOMOGENEIDADE GERAL
		// ------------------------------------------------------------------
		if (global_min_diameter != 9999) {
			int global_diameter_diff = global_max_diameter - global_min_diameter;

			int reference_caliber = get_caliber(global_max_diameter);
			int global_tolerance = -1;

			if (reference_caliber >= 0 && reference_caliber <= 2)
				global_tolerance = 11;
			else if (reference_caliber >= 3 && reference_caliber <= 6)
				global_tolerance = 9;
			else if (reference_caliber >= 7 && reference_caliber <= 13)
				global_tolerance = 7;

			std::string str_global_homo = "HOMOGENEIDADE GERAL: ";
			cv::Scalar color_status;

			if (global_tolerance > 0) {
				if (global_diameter_diff <= global_tolerance) {
					str_global_homo += "OK";
					color_status = cv::Scalar(0, 200, 0);
				}
				else {
					str_global_homo += "Not OK";
					color_status = cv::Scalar(0, 0, 255);
				}

				str_global_homo += " (Dif:" + std::to_string(global_diameter_diff) +
					"mm | Max Permitido:" + std::to_string(global_tolerance) + "mm)";
			}
			else {
				str_global_homo += "CALIBRE INVALIDO";
				color_status = cv::Scalar(0, 255, 255);
			}

			cv::putText(frame, str_global_homo, cv::Point(20, 230), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 3);
			cv::putText(frame, str_global_homo, cv::Point(20, 230), cv::FONT_HERSHEY_SIMPLEX, 0.6, color_status, 2);
		}

		// ------------------------------------------------------------------
		// MOSTRAR IMAGENS NO ECRAN
		// ------------------------------------------------------------------
		cv::Mat frameMask(video.height, video.width, CV_8UC1, image_temp2->data);

		cv::imshow("DETECAO DE LARANJAS", frame);
		cv::imshow("MASCARA BINARIA", frameMask);

		key = cv::waitKey(1);
	}

	// ------------------------------------------------------------------
	// LIBERTAR MEMORIA
	// ------------------------------------------------------------------
	cv::destroyAllWindows();
	capture.release();

	vc_image_free(image_src);
	vc_image_free(image_hsv);
	vc_image_free(image_mask);
	vc_image_free(image_temp1);
	vc_image_free(image_temp2);
	vc_image_free(image_labels);
	vc_image_free(image_good_colour);

	return 0;
}