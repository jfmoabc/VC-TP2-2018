#define VC_DEBUG
#define MAX(a,b) (a>b ? a:b)
#define MIN(a,b) (a<b ? a:b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char *data;
	int width, height;
	int channels;			// Binário/Cinzentos=1; RGB=3
	int levels;				// Binário=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// Área
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Perímetro
	int label;					// Etiqueta
} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);

// FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);

// CONVERSÃO RGB TO GRAY
int vc_rgb_to_gray(IVC *src, IVC *dst);

// Gerar negativo de imagem Gray
int vc_gray_negative(IVC *srcdst);

int vc_gray_to_binary(IVC *src, IVC *dst, int threshold);

// CONVERSÃO RGB TO HSV
int vc_rgb_to_hsv(IVC *srcdst);

// ERODE
int vc_binary_erode(IVC *src, IVC *dst, int kernel);

// DILATE
int vc_binary_dilate(IVC *src, IVC *dst, int kernel);

// OPEN
int vc_binary_open(IVC *src, IVC *dst, int kernel, int kernel2);

// BINARY CLOSE
int vc_binary_close(IVC *src, IVC *dst, int kernel, int kernel2);

int vc_coin_segmentation(IVC *srcdst);

OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels);


int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs);

int vc_rgb_to_bgr(IVC *src, IVC *dst);

int vc_bgr_to_rgb(IVC *src, IVC *dst);


int vc_iplImage_to_ivc(IplImage *src, IVC *dst);

int vc_ivc_to_iplImage(IVC *src, IplImage *dst);

int vc_limites(IplImage *srcdst, int high, int low);

int vc_bounding_box_IplImage(IplImage *srcdst, OVC *blobs, int nBlobs);