/**********************************************************************
* File:        tessedit.cpp  (Formerly tessedit.c)
* Description: Main program for merge of tess and editor.
* Author:                  Ray Smith
* Created:                 Tue Jan 07 15:21:46 GMT 1992
*
* (C) Copyright 1992, Hewlett-Packard Ltd.
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** http://www.apache.org/licenses/LICENSE-2.0
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*
**********************************************************************/

// Include automatically generated configuration file if running autoconf
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif

#include <iostream>

#include "allheaders.h"
#include "baseapi.h"
#include "basedir.h"
#include "renderer.h"
#include "strngs.h"
#include "tprintf.h"
#include "openclwrapper.h"
#include "osdetect.h"
#include "pageiterator.h"
#include "publictypes.h"

Pix* pre_process(Pix* pix, int max_size) {
	if (NULL == pix) {
		return NULL;
	}
	int bigger = pix->w > pix->h ? pix->w : pix->h;
	if (bigger <= max_size) {
		return pix;
	}
	float scale = max_size*1.0 / bigger;
	Pix* scaled_pix = pixScale(pix, scale, scale);
	pixDestroy(&pix);
	return scaled_pix;
}

string getLinesCode(tesseract::TessBaseAPI& api) {
	Pixa* pa = pixaCreate(1);
	pixaAddPix(pa, pix, L_INSERT);
	Boxa* regions = api.GetTextlines(true, 0, &pa, 0, 0);
}

void AnalyseLayout_test(tesseract::TessBaseAPI& api, const char* image, unsigned int threshold) {
	Pix* pix = pre_process(pixRead(image), 2000);
	if (!pix) {
		return ;
	}

	Pixa* pa = pixaCreate(1);
	pixaAddPix(pa, pix, L_INSERT);

	api.SetImage(pix);
	tesseract::PageIterator* it = api.AnalyseLayout();

	int l, t, r, b;
	Boxa* regions = api.GetTextlines(true, 0, &pa, 0, 0);
	Boxa* symbols = api.GetComponentImages(tesseract::RIL_SYMBOL, true, true, 0, &pa, 0, 0);
	/*while (it->BoundingBox(tesseract::RIL_BLOCK, &l, &t, &r, &b)) {
		while (it->BoundingBox(tesseract::RIL_PARA, &l, &t, &r, &b)) {
			while (it->BoundingBox(tesseract::RIL_TEXTLINE, &l, &t, &r, &b)) {
				while (it->BoundingBox(tesseract::RIL_WORD, &l, &t, &r, &b)) {
					//while (it->BoundingBox(tesseract::RIL_SYMBOL, &l, &t, &r, &b)) {
						boxaAddBox(symbols, boxCreate(l, t, r-l, b-t), L_INSERT);
					//	it->Next(tesseract::RIL_SYMBOL);
					//}
					it->Next(tesseract::RIL_WORD);
				}
				it->Next(tesseract::RIL_TEXTLINE);
			}
			it->Next(tesseract::RIL_PARA);
		}
		it->Next(tesseract::RIL_BLOCK);
	}*/ 

	Pix* pix_gray = pixConvertTo8(pix, 0);
	for (int i = 0; i < regions->n && 0; i++) {
		Box* b = regions->box[i];
		printf("BoundingBox:%d %d %d %d\n", b->x, b->y, b->w, b->h);
		
		//feature1: sum of pixel
		printf("sum of pixel:");
		for (int x = b->x; x < b->x + b->w; x++) {
			int sum = 0;
			for (int y = b->y; y < b->y + b->h; y++) {
				unsigned int v;
				pixGetPixel(pix_gray, x, y, &v);
				sum += 255 - v;
			}
			printf(" %d", sum/256);
		}
		printf("\n");

		//feature2: word height in this x coordinate
		printf("height in this x coordinate:");
		for (int x = b->x; x < b->x + b->w; x++) {
			int begin = -1, end = -1;
			for (int y = b->y; y < b->y + b->h; y++) {
				unsigned int v;
				pixGetPixel(pix_gray, x, y, &v);
				if (begin == -1 && v < threshold) 
					begin = y;
				if (v < threshold)
					end = y;
			}
			printf(" %d", end - begin);
		}
		printf("\n");
	}

	char outname[256];
	strcpy(outname, image);
	Pix* pixMar = pixDrawBoxa(pix, symbols, 1, 0x00FF0000);
	char* p = strrchr(outname, '.');
	if (p) {
		*p = 0;
		strcat(outname, "Ana");
		strcat(outname, ".jpg");
		printf("output filename:%s\n", outname);
		pixWrite(outname, pixMar, IFF_JFIF_JPEG);
	}
	pixDestroy(&pix);
	pixDestroy(&pix_gray);
}

void test(tesseract::TessBaseAPI& api, const char* image) {
  Pix* pix = pixRead(image);
  if (!pix) {
	  return ;
  }
  Pixa* pa = pixaCreate(1);
  pixaAddPix(pa, pix, L_INSERT);

  api.SetImage(pix);
  Boxa* regions = api.GetRegions(&pa);
  if (!regions)
	  return ;
  for (int i = 0; i < regions->n; i++) {
	Box* b = regions->box[i];
	printf("%d %d %d %d\n", b->x, b->y, b->w, b->h);
  }
  Pix* pixMar = pixDrawBoxa(pix, regions, 5, 0x77777777);
  char outname[256];
  strcpy(outname, image);
  char* p = strrchr(outname, '.');
  if (p) {
	  *p = 0;
	  strcat(outname, "Mar");
	  strcat(outname, strrchr(image, '.'));
	  printf("output filename:%s\n", outname);
	  pixWrite(outname, pixMar, IFF_JFIF_JPEG);
  }
}

/*
	get picture feature main
	author:nijun
	date:2015.11.30
*/
int main_bak(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Wrong arguments!\n");
		return -1;
	}
	tesseract::TessBaseAPI api;
	Pix* pix = pre_process(pixRead(argv[1]), 3000);
	if (NULL == pix)
		return -1;
	api.SetImage(pix);
	return 0;
}

/**********************************************************************
 *  main()
 *
 **********************************************************************/

int main(int argc, char **argv) {
	printf("inijun\n");
  if ((argc == 2 && strcmp(argv[1], "-v") == 0) ||
      (argc == 2 && strcmp(argv[1], "--version") == 0)) {
    char *versionStrP;

    fprintf(stderr, "tesseract %s\n", tesseract::TessBaseAPI::Version());

    versionStrP = getLeptonicaVersion();
    fprintf(stderr, " %s\n", versionStrP);
    lept_free(versionStrP);

    versionStrP = getImagelibVersions();
    fprintf(stderr, "  %s\n", versionStrP);
    lept_free(versionStrP);

#ifdef USE_OPENCL
    cl_platform_id platform;
    cl_uint num_platforms;
    cl_device_id devices[2];
    cl_uint num_devices;
    char info[256];
    int i;

    fprintf(stderr, " OpenCL info:\n");
    clGetPlatformIDs(1, &platform, &num_platforms);
    fprintf(stderr, "  Found %d platforms.\n", num_platforms);
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, 256, info, 0);
    fprintf(stderr, "  Platform name: %s.\n", info);
    clGetPlatformInfo(platform, CL_PLATFORM_VERSION, 256, info, 0);
    fprintf(stderr, "  Version: %s.\n", info);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 2, devices, &num_devices);
    fprintf(stderr, "  Found %d devices.\n", num_devices);
    for (i = 0; i < num_devices; ++i) {
      clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 256, info, 0);
      fprintf(stderr, "    Device %d name: %s.\n", i+1, info);
    }
#endif
    exit(0);
  }

  // Make the order of args a bit more forgiving than it used to be.
  const char* lang = "eng";
  const char* image = NULL;
  const char* outputbase = NULL;
  const char* datapath = NULL;
  bool noocr = false;
  bool list_langs = false;
  bool print_parameters = false;
  GenericVector<STRING> vars_vec, vars_values;

  tesseract::PageSegMode pagesegmode = tesseract::PSM_AUTO;
  int arg = 1;
  while (arg < argc && (outputbase == NULL || argv[arg][0] == '-')) {
    if (strcmp(argv[arg], "-l") == 0 && arg + 1 < argc) {
      lang = argv[arg + 1];
      ++arg;
    } else if (strcmp(argv[arg], "--tessdata-dir") == 0 && arg + 1 < argc) {
      datapath = argv[arg + 1];
      ++arg;
    } else if (strcmp(argv[arg], "--user-words") == 0 && arg + 1 < argc) {
      vars_vec.push_back("user_words_file");
      vars_values.push_back(argv[arg + 1]);
      ++arg;
    } else if (strcmp(argv[arg], "--user-patterns") == 0 && arg + 1 < argc) {
      vars_vec.push_back("user_patterns_file");
      vars_values.push_back(argv[arg + 1]);
      ++arg;
    } else if (strcmp(argv[arg], "--list-langs") == 0) {
      noocr = true;
      list_langs = true;
    } else if (strcmp(argv[arg], "-psm") == 0 && arg + 1 < argc) {
      pagesegmode = static_cast<tesseract::PageSegMode>(atoi(argv[arg + 1]));
      ++arg;
    } else if (strcmp(argv[arg], "--print-parameters") == 0) {
      noocr = true;
      print_parameters = true;
    } else if (strcmp(argv[arg], "-c") == 0 && arg + 1 < argc) {
      // handled properly after api init
      ++arg;
    } else if (image == NULL) {
      image = argv[arg];
    } else if (outputbase == NULL) {
      outputbase = argv[arg];
    }
    ++arg;
  }

  if (argc == 2 && strcmp(argv[1], "--list-langs") == 0) {
    list_langs = true;
    noocr = true;
  }

  if (outputbase == NULL && noocr == false) {
    fprintf(stderr, "Usage:\n  %s imagename|stdin outputbase|stdout "
            "[options...] [configfile...]\n\n", argv[0]);

    fprintf(stderr, "OCR options:\n");
    fprintf(stderr, "  --tessdata-dir /path\tspecify the location of tessdata"
                      " path\n");
    fprintf(stderr, "  --user-words /path/to/file\tspecify the location of user"
            " words file\n");
    fprintf(stderr, "  --user-patterns /path/to/file\tspecify the location of"
            " user patterns file\n");
    fprintf(stderr, "  -l lang[+lang]\tspecify language(s) used for OCR\n");
    fprintf(stderr, "  -c configvar=value\tset value for control parameter.\n"
                      "\t\t\tMultiple -c arguments are allowed.\n");
    fprintf(stderr, "  -psm pagesegmode\tspecify page segmentation mode.\n");
    fprintf(stderr, "These options must occur before any configfile.\n\n");
    fprintf(stderr,
            "pagesegmode values are:\n"
            "  0 = Orientation and script detection (OSD) only.\n"
            "  1 = Automatic page segmentation with OSD.\n"
            "  2 = Automatic page segmentation, but no OSD, or OCR\n"
            "  3 = Fully automatic page segmentation, but no OSD. (Default)\n"
            "  4 = Assume a single column of text of variable sizes.\n"
            "  5 = Assume a single uniform block of vertically aligned text.\n"
            "  6 = Assume a single uniform block of text.\n"
            "  7 = Treat the image as a single text line.\n"
            "  8 = Treat the image as a single word.\n"
            "  9 = Treat the image as a single word in a circle.\n"
            "  10 = Treat the image as a single character.\n\n");
    fprintf(stderr, "Single options:\n");
    fprintf(stderr, "  -v --version: version info\n");
    fprintf(stderr, "  --list-langs: list available languages for tesseract "
                      "engine. Can be used with --tessdata-dir.\n");
    fprintf(stderr, "  --print-parameters: print tesseract parameters to the "
                      "stdout.\n");
    exit(1);
  }

  if (outputbase != NULL && strcmp(outputbase, "-") &&
      strcmp(outputbase, "stdout")) {
    tprintf("Tesseract Open Source OCR Engine v%s with Leptonica\n",
           tesseract::TessBaseAPI::Version());
  }
  PERF_COUNT_START("Tesseract:main")
  tesseract::TessBaseAPI api;

  api.SetOutputName(outputbase);
  int rc = api.Init(datapath, lang, tesseract::OEM_DEFAULT,
                &(argv[arg]), argc - arg, &vars_vec, &vars_values, false);

  if (rc) {
    fprintf(stderr, "Could not initialize tesseract.\n");
    exit(1);
  }

  char opt1[255], opt2[255];
  for (arg = 0; arg < argc; arg++) {
    if (strcmp(argv[arg], "-c") == 0 && arg + 1 < argc) {
      strncpy(opt1, argv[arg + 1], 255);
      char *p = strchr(opt1, '=');
      if (!p) {
        fprintf(stderr, "Missing = in configvar assignment\n");
        exit(1);
      }
      *p = 0;
      strncpy(opt2, strchr(argv[arg + 1], '=') + 1, 255);
      opt2[254] = 0;
      ++arg;

      if (!api.SetVariable(opt1, opt2)) {
        fprintf(stderr, "Could not set option: %s=%s\n", opt1, opt2);
      }
    }
  }

  if (list_langs) {
     GenericVector<STRING> languages;
     api.GetAvailableLanguagesAsVector(&languages);
     fprintf(stderr, "List of available languages (%d):\n",
             languages.size());
     for (int index = 0; index < languages.size(); ++index) {
       STRING& string = languages[index];
       fprintf(stderr, "%s\n", string.string());
     }
     api.End();
     exit(0);
  }

  if (print_parameters) {
     FILE* fout = stdout;
     fprintf(stdout, "Tesseract parameters:\n");
     api.PrintVariables(fout);
     api.End();
     exit(0);
  }

  // We have 2 possible sources of pagesegmode: a config file and
  // the command line. For backwards compatibility reasons, the
  // default in tesseract is tesseract::PSM_SINGLE_BLOCK, but the
  // default for this program is tesseract::PSM_AUTO. We will let
  // the config file take priority, so the command-line default
  // can take priority over the tesseract default, so we use the
  // value from the command line only if the retrieved mode
  // is still tesseract::PSM_SINGLE_BLOCK, indicating no change
  // in any config file. Therefore the only way to force
  // tesseract::PSM_SINGLE_BLOCK is from the command line.
  // It would be simpler if we could set the value before Init,
  // but that doesn't work.
  if (api.GetPageSegMode() == tesseract::PSM_SINGLE_BLOCK)
     api.SetPageSegMode(pagesegmode);

  if (pagesegmode == tesseract::PSM_AUTO_ONLY ||
      pagesegmode == tesseract::PSM_OSD_ONLY) {
    int ret_val = 0;

    Pix* pixs = pixRead(image);
    if (!pixs) {
      fprintf(stderr, "Cannot open input file: %s\n", image);
      exit(2);
    }
    api.SetImage(pixs);

    if (pagesegmode == tesseract::PSM_OSD_ONLY) {
       OSResults osr;
       if (api.DetectOS(&osr)) {
         int orient = osr.best_result.orientation_id;
         int script_id = osr.get_best_script(orient);
         float orient_oco = osr.best_result.oconfidence;
         float orient_sco = osr.best_result.sconfidence;
         tprintf("Orientation: %d\nOrientation in degrees: %d\n" \
                 "Orientation confidence: %.2f\n" \
                 "Script: %d\nScript confidence: %.2f\n",
                 orient, OrientationIdToValue(orient), orient_oco,
                 script_id, orient_sco);
       } else {
         ret_val = 1;
       }
    } else {
       tesseract::Orientation orientation;
       tesseract::WritingDirection direction;
       tesseract::TextlineOrder order;
       float deskew_angle;
       tesseract::PageIterator* it =  api.AnalyseLayout();
       if (it) {
         it->Orientation(&orientation, &direction, &order, &deskew_angle);
         tprintf("Orientation: %d\nWritingDirection: %d\nTextlineOrder: %d\n" \
                 "Deskew angle: %.4f\n",
                 orientation, direction, order, deskew_angle);
       } else {
         ret_val = 1;
       }
       delete it;
    }
    pixDestroy(&pixs);
    exit(ret_val);
  }

  bool b;
  tesseract::PointerVector<tesseract::TessResultRenderer> renderers;
  api.GetBoolVariable("tessedit_create_hocr", &b);
  if (b) {
    bool font_info;
    api.GetBoolVariable("hocr_font_info", &font_info);
    renderers.push_back(new tesseract::TessHOcrRenderer(outputbase, font_info));
  }
  api.GetBoolVariable("tessedit_create_pdf", &b);
  if (b) {
    renderers.push_back(new tesseract::TessPDFRenderer(outputbase,
                                                       api.GetDatapath()));
  }
  api.GetBoolVariable("tessedit_write_unlv", &b);
  if (b) renderers.push_back(new tesseract::TessUnlvRenderer(outputbase));
  api.GetBoolVariable("tessedit_create_boxfile", &b);
  if (b) renderers.push_back(new tesseract::TessBoxTextRenderer(outputbase));
  api.GetBoolVariable("tessedit_create_txt", &b);
  if (b) renderers.push_back(new tesseract::TessTextRenderer(outputbase));
  if (!renderers.empty()) {
    // Since the PointerVector auto-deletes, null-out the renderers that are
    // added to the root, and leave the root in the vector.
    for (int r = 1; r < renderers.size(); ++r) {
      renderers[0]->insert(renderers[r]);
      renderers[r] = NULL;
    }
    /*if (!api.ProcessPages(image, NULL, 0, renderers[0])) {
      fprintf(stderr, "Error during processing.\n");
      exit(1);
    }*/
	
  }
  AnalyseLayout_test(api, image, 128);
  PERF_COUNT_END
  return 0;                      // Normal exit
}
