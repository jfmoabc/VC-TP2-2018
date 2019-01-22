#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <opencv\cv.h>
#include <opencv\cxcore.h>
#include <opencv\highgui.h>
#include "vc.h"

#define N 5

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Alocar memória para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}

// Libertar memória de uma imagem
IVC *vc_image_free(IVC *image)
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

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)));
		if (c != '#') break;
		do c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF) break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}

long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y<height; y++)
	{
		for (x = 0; x<width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}

void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for (y = 0; y<height; y++)
	{
		for (x = 0; x<width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}

IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0, lab(level,255)])
		else if (strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}

int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: EDIÇÃO DE IMAGENS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Gerar imagem em escala de cinzentos a partir de imagem RGB	
int vc_rgb_to_gray(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	//verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;


	//Passa os pixéis da escala RGB para escala de cinzentos
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src + 1];
			bf = (float)datasrc[pos_src + 2];

			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}
	return 1;
}

// Gerar negativo de imagem Gray
int vc_gray_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Inverte a imagem Gray
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];
		}
	}
	return 1;
}

// COVERTER RGB PARA HSV
int vc_rgb_to_hsv(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	// Verificaçao de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Calcula valores maximo e minimo dos canais de cor R, G e B
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value == 0.0)
		{
			hue = 0.0;
			saturation = 0.0;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0)
			{
				hue = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else /* rgb_max == b*/
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}

		// Atribui valores entre [0,255]
		data[i] = (unsigned char)(hue / 360.0 * 255.0);
		data[i + 1] = (unsigned char)(saturation);
		data[i + 2] = (unsigned char)(value);
	}
	return 1;
}

int vc_gray_to_binary(IVC *src, IVC *dst, int threshold)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	long int t, t2;

	//verificar erros
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 1) return 0;

	t = 0;
	//soma do valor total dos pixeis
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			t += data[pos];
		}
	}

	//t2 = (int)(((float)t)/((float)width*height));

	t2 = t / (width*height);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if (data[pos] > threshold)
			{
				dst->data[pos] = 255;
			}
			else
			{
				dst->data[pos] = 0;
			}
		}
	}
	return 1;
}

// BINARY DILATE
int vc_binary_dilate(IVC *src, IVC *dst, int kernel)
{
	if (src->height != dst->height) return -1;
	if (src->width != dst->width) return -1;
	if (src->channels != dst->channels) return -1;

	int y, x, pos;
	int yy, xx, posk;
	int posicao = (kernel / 2);

	//Utilizando o kernel fornecido converte os pontos na vizinhança do ponto em que está para a cor deste
	for (y = posicao; y < (src->height - posicao); y++)
	{
		for (x = posicao; x < (src->width - posicao); x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
			//dst->data[pos + 1] = 0;
			//dst->data[pos + 2] = 0;

			for (yy = -posicao; yy <= posicao; yy++)
			{
				for (xx = -posicao; xx <= posicao; xx++)
				{
					posk = (y - yy) * src->bytesperline + (x - xx) * src->channels;

					if (src->data[posk] != 0)
					{
						dst->data[pos] = 255;
						//dst->data[pos + 1] = 255;
						//dst->data[pos + 2] = 255;
					}
				}
			}
		}
	}

	//Remove o "border" deixado pela operação anterior
	for (y = 0; y <= posicao; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
		}
	}
	for (y = src->height - posicao - 1; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
		}
	}
	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x <= posicao; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
		}
	}
	for (y = 0; y < src->height; y++)
	{
		for (x = src->width - posicao - 1; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
		}
	}
	return 1;
}

// BINARY ERODE
int vc_binary_erode(IVC *src, IVC *dst, int kernel)
{
	//Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (src->channels != 1) return 0;

	int y, x, pos, yy, xx, posk;
	int posicao = kernel / 2;

	for (y = posicao; y < (src->height - posicao); y++)
	{
		for (x = posicao; x < (src->width - posicao); x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 255;

			for (yy = -posicao; yy <= posicao; yy++)
			{
				for (xx = -posicao; xx <= posicao; xx++)
				{
					posk = (y - yy) * src->bytesperline + (x - xx) * src->channels;

					if (src->data[posk] == 0)
					{
						dst->data[pos] = 0;
					}
				}
			}
		}
	}

	//Remove o "border" deixado pela operação anterior
	for (y = 0; y <= posicao; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
		}
	}
	for (y = src->height - posicao - 1; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
		}
	}
	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x <= posicao; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
		}
	}
	for (y = 0; y < src->height; y++)
	{
		for (x = src->width - posicao - 1; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			dst->data[pos] = 0;
		}
	}
	return 1;
}

// BINARY OPEN
int vc_binary_open(IVC *src, IVC *dst, int kernel, int kernel2)
{
	IVC *tmp;
	tmp = vc_image_new(src->width, src->height, 1, src->levels);

	vc_binary_erode(src, tmp, kernel);
	vc_binary_dilate(tmp, dst, kernel2);
	vc_image_free(tmp);

	return 1;
}

// BINARY CLOSE
int vc_binary_close(IVC *src, IVC *dst, int kernel, int kernel2)
{
	IVC *tmp;
	tmp = vc_image_new(src->width, src->height, 1, src->levels);

	vc_binary_dilate(src, tmp, kernel);
	vc_binary_erode(tmp, dst, kernel2);
	vc_image_free(tmp);

	return 1;
}

// IPLIMAGE TO IVC
int vc_iplImage_to_ivc(IplImage *src, IVC *dst)
{
	int x, y;
	long int pos;

	if ((dst->width != src->width || dst->height != src->height) && (dst->channels != 3 || src->nChannels != 3)) return 0;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * dst->bytesperline + x * dst->channels;

			dst->data[pos] = src->imageData[pos];
			dst->data[pos + 1] = src->imageData[pos + 1];
			dst->data[pos + 2] = src->imageData[pos + 2];
		}
	}
	return 1;
}

// IVC TO IPLIMAGE
int vc_ivc_to_iplImage(IVC *src, IplImage *dst)
{
	int x, y;
	long int pos;

	if ((dst->width != src->width || dst->height != src->height) && (src->channels != 3 || dst->nChannels != 3)) return 0;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;

			dst->imageData[pos] = src->data[pos];
			dst->imageData[pos + 1] = src->data[pos];
			dst->imageData[pos + 2] = src->data[pos];
		}
	}
	return 1;
}

// RGB TO BGR
int vc_rgb_to_bgr(IVC *src, IVC *dst)
{
	int Red, Green, Blue;
	int i;

	// Verificação de erro
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (src->channels != 3) return 0; // so para qd é cor

	for (i = 0; i < src->bytesperline*src->height; i += src->channels)
	{
		Red = src->data[i];
		Green = src->data[i + 1];
		Blue = src->data[i + 2];

		dst->data[i] = Blue;
		dst->data[i + 1] = Green;
		dst->data[i + 2] = Red;
	}
	//fim
	return 1;
}

// BGR TO RGB
int vc_bgr_to_rgb(IVC *src, IVC *dst)
{
	int Red, Green, Blue;
	int i;

	// Verificação de erro
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (src->channels != 3) return 0; // so para qd é cor

	for (i = 0; i < src->bytesperline*src->height; i += src->channels)
	{
		Blue = src->data[i];
		Green = src->data[i + 1];
		Red = src->data[i + 2];

		dst->data[i] = Red;
		dst->data[i + 1] = Green;
		dst->data[i + 2] = Blue;
	}
	//fim
	return 1;
}

// SEGMENTA AS MOEDAS POR COR
int vc_coin_segmentation(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width*srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;


	//Devolve uma imagem só com fundo de uma cor e os pixeis vermelhos de outra
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if ((data[pos] > 10 && data[pos] < 70) 
				&& (data[pos + 1] > 3) 
				&& (data[pos + 2] > 10 && data[pos + 2] < 130)
				)
			{
				data[pos] = 255;
				data[pos + 1] = 255;
				data[pos + 2] = 255;
			}
			else
			{
				data[pos] = 0;
				data[pos + 1] = 0;
				data[pos + 2] = 0;
			}
		}
	}
	return 1;
}

// Desenha a Bounding Box e o Centro de Gravidade
int vc_bounding_box_IplImage(IplImage *srcdst, OVC *blobs, int nBlobs)
{
	int i, x, y;
	long int pos;

	//Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->imageData == NULL)) return 0;

	for (int i = 0; i < nBlobs; i++)
	{
		for (y = 0; y < srcdst->height; y++)
		{
			for (x = 0; x < srcdst->width; x++)
			{
				pos = y * srcdst->widthStep + x * srcdst->nChannels;

				// Desenha a Bounding Box
				if (((x >= blobs[i].x && x <= blobs[i].x + blobs[i].width) && (y == blobs[i].y || y == blobs[i].y + blobs[i].height)) || ((x == blobs[i].x || x == blobs[i].x + blobs[i].width) && (y >= blobs[i].y && y <= blobs[i].y + blobs[i].height)))
				{
					srcdst->imageData[pos] = 255;
					srcdst->imageData[pos + 1] = 255;
					srcdst->imageData[pos + 2] = 255;
				}
				if (srcdst->nChannels == 3)
				{
					// Desenha Centro de Massa
					if ((x == blobs[i].xc) && (y == blobs[i].yc))
					{
						srcdst->imageData[pos] = 255;
						srcdst->imageData[pos + 1] = 255;
						srcdst->imageData[pos + 2] = 255;
					}
				}
				else if (srcdst->nChannels == 1)
				{
					// Desenha Centro de Massa
					if ((x == blobs[i].xc) && (y == blobs[i].yc))
					{
						srcdst->imageData[pos] = 255;
					}
				}
			}
		}
	}
	return 1;
}

// Desenha a Área onde obtem info dos blobs
int vc_limites(IplImage *srcdst, int high, int low)
{
	int x, y;
	long int pos;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->imageData == NULL)) return 0;
	if (srcdst->nChannels != 3) return 0;

	for (y = 0; y < srcdst->height; y++)
	{
		for (x = 0; x < srcdst->width; x++)
		{
			if (y == high || y == low)
			{
				pos = y * srcdst->widthStep + x * srcdst->nChannels;

				srcdst->imageData[pos] = 255;
				srcdst->imageData[pos + 1] = 255;
				srcdst->imageData[pos + 2] = 255;
			}
		}
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: ETIQUETAGEM DE BLOBS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o número de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. É necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC *blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 255 labels
	for (i = 0, size = bytesperline * height; i<size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y<height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x<width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y<height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

													// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b<label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a<label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC *)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a<(*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta área de cada blob
	for (i = 0; i<nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y<height - 1; y++)
		{
			for (x = 1; x<width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno	
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

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}
	return 1;
}

