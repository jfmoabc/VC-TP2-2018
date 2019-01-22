#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>

#include <opencv\cv.h>
#include <opencv\cxcore.h>
#include <opencv\highgui.h>

extern "C"
{
#include "vc.h"
}

int main(void)
{
	// Vídeo
	char videofile[50] = "video1-tp2.mp4";
	CvCapture *capture;
	IplImage *frame;

	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	// Texto
	CvFont font, fontbkg;
	double hScale = 0.5;
	double vScale = 0.5;
	int lineWidth = 1;
	char str[500] = { 0 };

	// Outros
	int key = 0;
	int nObjetos = 0, nObjetosPrev = 0;
	int x, y, i, j;
	int contapixeis;
	int aObjetos;
	int upper = 500;
	int lower = 550;
	int c1 = 0, c2 = 0, c5 = 0, c10 = 0, c20 = 0, c50 = 0, e1 = 0, e2 = 0, cont = 0;
	int flag = 0;
	int area;
	float ratio;
	float divisao[40];
	float perimetro;
	long int pos;
	IVC *image[5];
	IVC *imageBin[5];
	OVC *objetos, *objetosPrev = NULL;


	/* Leitura de vídeo de um ficheiro */
	capture = cvCaptureFromFile(videofile);

	/* Verifica se foi possível abrir o ficheiro de vídeo */
	if (!capture)
	{
		fprintf(stderr, "Erro ao abrir o ficheiro de vídeo!\n");
		return 1;
	}

	/* Número total de frames no vídeo */
	video.ntotalframes = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
	/* Frame rate do vídeo */
	video.fps = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
	/* Resolução do vídeo */
	video.width = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	video.height = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o vídeo */
	cvNamedWindow("VC - TP2", CV_WINDOW_AUTOSIZE);

	/* Inicializa a fonte */
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, hScale, vScale, 0, lineWidth);
	cvInitFont(&fontbkg, CV_FONT_HERSHEY_SIMPLEX, hScale, vScale, 0, lineWidth + 1);


	// Cria Array de Imagens
	for (i = 0; i < 5; i++)
	{
		image[i] = vc_image_new(video.width, video.height, 3, 255);
	}
	for (i = 0; i < 5; i++)
	{
		imageBin[i] = vc_image_new(video.width, video.height, 1, 255);
	}


	while (key != 'q') 
	{
		/* Leitura de uma frame do vídeo */
		frame = cvQueryFrame(capture);

		/* Verifica se conseguiu ler a frame */
		if (!frame) break;

		/* Número da frame a processar */
		video.nframe = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES);

		/* Exemplo de inserção texto na frame */
		sprintf(str, "RESOLUCAO: %dx%d", video.width, video.height);
		cvPutText(frame, str, cvPoint(20, 20), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 20), &font, cvScalar(255, 255, 255));
		sprintf(str, "TOTAL DE FRAMES: %d", video.ntotalframes);
		cvPutText(frame, str, cvPoint(20, 40), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 40), &font, cvScalar(255, 255, 255));
		sprintf(str, "FRAME RATE: %d", video.fps);
		cvPutText(frame, str, cvPoint(20, 60), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 60), &font, cvScalar(255, 255, 255));
		sprintf(str, "N. FRAME: %d", video.nframe);
		cvPutText(frame, str, cvPoint(20, 80), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 80), &font, cvScalar(255, 255, 255));

		// Converter a frame para IVC
		vc_iplImage_to_ivc(frame, image[0]);

		// Troca canais
		vc_bgr_to_rgb(image[0], image[1]);

		// Troca de espaço de cor
		vc_rgb_to_hsv(image[1]);

		// Segmenta a imagem para apresentar apenas as moedas
		vc_coin_segmentation(image[1]);

		// Transforma em escala de cinzentos
		vc_rgb_to_gray(image[1], imageBin[0]);

		// Transforma em binário
		vc_gray_to_binary(imageBin[0], imageBin[1], 120);

		// Dilata o binário
		vc_binary_dilate(imageBin[1], imageBin[2], 7);

		// Identificar os blobs
		objetos = vc_binary_blob_labelling(imageBin[2], imageBin[3], &nObjetos);
		vc_binary_blob_info(imageBin[3], objetos, nObjetos);

		if (nObjetos != 0)
		{
			for (int i = 0; i < nObjetos; i++)
			{
				// Remove blobs demasiado grandes
				if (objetos[i].area < 9000)
				{
					for (y = objetos[i].y; y < objetos[i].y + objetos[i].height; y++)
					{
						for (x = objetos[i].x; x < objetos[i].x + objetos[i].width; x++)
						{
							pos = y * imageBin[2]->bytesperline + x * imageBin[2]->channels;
							imageBin[2]->data[pos] = 0;
						}
					}
				}
			}

			// Identificar os blobs
			objetos = vc_binary_blob_labelling(imageBin[2], imageBin[3], &nObjetos);
			vc_binary_blob_info(imageBin[3], objetos, nObjetos);

			// retira os pixeis que tem menos do que 70% de pixeis brancos
			for (i = 0; i < nObjetos; i++)
			{
				for (y = objetos[i].y; y < objetos[i].y + objetos[i].height; y++)
				{
					for (x = objetos[i].x; x < objetos[i].x + objetos[i].width; x++)
					{
						pos = y * imageBin[2]->bytesperline + x * imageBin[2]->channels;

						if (imageBin[2]->data[pos] != 0)
						{
							contapixeis += 1;
						}

					}
				}

				//++++++++++++++++++++++++++++++++++++++++++++++++++++++
				// TROCAR CONTAPIXEIS POR IMAGEBIN[2].AREA
				//++++++++++++++++++++++++++++++++++++++++++++++++++++++

				aObjetos = objetos[i].width*objetos[i].height;
				divisao[i] = (float)contapixeis / (float)aObjetos;

				if (divisao[i] < 0.5)
				{
					for (y = objetos[i].y; y < objetos[i].y + objetos[i].height; y++)
					{
						for (x = objetos[i].x; x < objetos[i].x + objetos[i].width; x++)
						{
							pos = y * imageBin[2]->bytesperline + x * imageBin[2]->channels;
							imageBin[2]->data[pos] = 0;
						}
					}
				}
				contapixeis = 0;
			}


			// Identificar os blobs
			objetos = vc_binary_blob_labelling(imageBin[2], imageBin[3], &nObjetos);
			vc_binary_blob_info(imageBin[3], objetos, nObjetos);

			// Remover blobs que não são quadrados
			for (i = 1; i < nObjetos; i++)
			{
				ratio = ((float)objetos[i].width / (float)objetos[i].height);

				//se a divisão for muito diferente de 1
				if (ratio < 0.8 || ratio > 1.2)
				{
					for (y = objetos[i].y; y < objetos[i].y + objetos[i].height; y++)
					{
						for (x = objetos[i].x; x < objetos[i].x + objetos[i].width; x++)
						{
							pos = y * imageBin[2]->bytesperline + x * imageBin[2]->channels;
							imageBin[2]->data[pos] = 0;
						}
					}
				}
				ratio = 0;
			}


			// Identificar os blobs
			objetos = vc_binary_blob_labelling(imageBin[2], imageBin[3], &nObjetos);
			vc_binary_blob_info(imageBin[3], objetos, nObjetos);

			if (objetos != NULL)
			{
				for (i = 0; i < nObjetos; i++)
				{
					if (objetos[i].yc > upper && objetos[i].yc < lower)
					{
						for (j = 0; j < nObjetosPrev; j++)
						{
							if (objetos[i].yc > objetosPrev[j].y && objetos[i].yc < objetosPrev[j].y + objetosPrev[j].height
								&& objetos[i].xc > objetosPrev[j].x && objetos[i].xc < objetosPrev[j].x + objetosPrev[j].width
								&& objetosPrev[j].yc > upper && objetosPrev[j].yc < lower
								)
							{
								flag = 1;
								break;
							}
						}
						if(flag == 0)
						{
								cont++;

								area = objetos[i].width * objetos[i].height;
								perimetro = (float)(objetos[i].width / 2) * 2 * 3.14;

								// 1 cent
								if (area > 15000 && area < 18000)
								{
									printf("\nMoeda nr %d:\n", cont);
									printf("\tValor: 1 centimo.\n");
									printf("\tArea: %d (pixeis).\n", objetos[i].area);
									printf("\tPerimetro: %.2f (pixeis).\n", perimetro);

									c1++;

								}
								// 2 cent
								if (area > 20000 && area < 21500)
								{
									printf("\nMoeda nr %d:\n", cont);
									printf("\tValor: 2 centimos.\n");
									printf("\tArea: %d (pixeis).\n", objetos[i].area);
									printf("\tPerimetro: %.2f (pixeis).\n", perimetro);

									c2++;
								}

								// 5 cent
								if (area > 25000 && area < 27500)
								{
									printf("\nMoeda nr %d:\n", cont);
									printf("\tValor: 5 centimos.\n");
									printf("\tArea: %d (pixeis).\n", objetos[i].area);
									printf("\tPerimetro: %.2f (pixeis).\n", perimetro);

									c5++;
								}
								// 10 cents
								if (area > 21500 && area < 25000)
								{
									printf("\nMoeda nr %d:\n", cont);
									printf("\tValor: 10 centimos.\n");
									printf("\tArea: %d (pixeis).\n", objetos[i].area);
									printf("\tPerimetro: %.2f (pixeis).\n", perimetro);

									c10++;
								}
								// 20 cent
								if (area > 27500 && area < 30000)
								{
									printf("\nMoeda nr %d:\n", cont);
									printf("\tValor: 20 centimos.\n");
									printf("\tArea: %d (pixeis).\n", objetos[i].area);
									printf("\tPerimetro: %.2f (pixeis).\n", perimetro);

									c20++;
								}
								// 50 cents
								if (area > 32000 && area < 35600)
								{
									printf("\nMoeda nr %d:\n", cont);
									printf("\tValor: 50 centimos.\n");
									printf("\tArea: %d (pixeis).\n", objetos[i].area);
									printf("\tPerimetro: %.2f (pixeis).\n", perimetro);
									
									c50++;
								}

								// 1 euro
								if (area > 30000 && area < 32000)
								{
									printf("\nMoeda nr %d:\n", cont);
									printf("\tValor: 1 euro.\n");
									printf("\tArea: %d (pixeis).\n", objetos[i].area);
									printf("\tPerimetro: %.2f (pixeis).\n", perimetro);

									e1++;
								}
								// 2 euros  
								if (area > 35600 && area < 40000)
								{
									printf("\nMoeda nr %d:\n", cont);
									printf("\tValor: 2 euros.\n");
									printf("\tArea: %d (pixeis).\n", objetos[i].area);
									printf("\tPerimetro: %.2f (pixeis).\n", perimetro);

									e2++;
								}
						}
						flag = 0;
					}
				}
			}
		}

		//vc_ivc_to_iplImage(imageBin[2	], frame);
		vc_limites(frame, upper, lower);

		if (objetos != NULL)
		{
			vc_bounding_box_IplImage(frame, objetos, nObjetos);
		}

		// Liberta Blobs da frame anterior
		if (objetosPrev != NULL)
		{
			free(objetosPrev);
		}

		// Atribui blobs atuais a blobs anteriores
		nObjetosPrev = nObjetos;
		objetosPrev = vc_binary_blob_labelling(imageBin[2], imageBin[3], &nObjetos);
		vc_binary_blob_info(imageBin[3], objetosPrev, nObjetosPrev);

		// Liberta blobs atuais
		if (objetos != NULL)
		{
			free(objetos);
		}


		/* Exibe a frame */
		cvShowImage("VC - TP2", frame);

		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
		key = cvWaitKey(1);
	}

	printf("\nTotal de moedas de 1 centimo: %d", c1);
	printf("\nTotal de moedas de 2 centimos: %d", c2);
	printf("\nTotal de moedas de 5 centimos: %d", c5);
	printf("\nTotal de moedas de 10 centimos: %d", c10);
	printf("\nTotal de moedas de 20 centimos: %d", c20);
	printf("\nTotal de moedas de 50 centimos: %d", c50);
	printf("\nTotal de moedas de 1 euro: %d", e1);
	printf("\nTotal de moedas de 2 euros: %d", e2);
	printf("\nTotal de moedas: %d", cont);
	printf("\n\n");

	/* Fecha a janela */
	cvDestroyWindow("VC - TP2");

	/* Fecha o ficheiro de vídeo */
	cvReleaseCapture(&capture);

	//system("Pause");
	return 0;
}