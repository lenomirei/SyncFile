void encode(FILE* inputFile, FILE* outputFile)
{
	/* set up a destination buffer large enough to hold the encoded data */
	int size = SIZE;
	char* input = (char*)malloc(size);
	char* encoded = (char*)malloc(2*size); /* ~4/3 x input */
	/* we need an encoder and decoder state */
	base64_encodestate es;
	/* store the number of bytes encoded by a single call */
	int cnt = 0;
	
	/*---------- START ENCODING ----------*/
	/* initialise the encoder state */
	base64_init_encodestate(&es);
	/* gather data from the input and send it to the output */
	while (1)
	{
		cnt = fread(input, sizeof(char), size, inputFile);
		if (cnt == 0) break;
		cnt = base64_encode_block(input, cnt, encoded, &es);
		/* output the encoded bytes to the output file */
		fwrite(encoded, sizeof(char), cnt, outputFile);
	}
	/* since we have reached the end of the input file, we know that 
	   there is no more input data; finalise the encoding */
	cnt = base64_encode_blockend(encoded, &es);
	/* write the last bytes to the output file */
	fwrite(encoded, sizeof(char), cnt, outputFile);
	/*---------- STOP ENCODING  ----------*/
	
	free(encoded);
	free(input);
}

void decode(FILE* inputFile, FILE* outputFile)
{
	/* set up a destination buffer large enough to hold the decoded data */
	int size = SIZE;
	char* encoded = (char*)malloc(size);
	char* decoded = (char*)malloc(1*size); /* ~3/4 x encoded */
	/* we need an encoder and decoder state */
	base64_decodestate ds;
	/* store the number of bytes encoded by a single call */
	int cnt = 0;
	
	/*---------- START DECODING ----------*/
	/* initialise the encoder state */
	base64_init_decodestate(&ds);
	/* gather data from the input and send it to the output */
	while (1)
	{
		cnt = fread(encoded, sizeof(char), size, inputFile);
		if (cnt == 0) break;
		cnt = base64_decode_block(encoded, cnt, decoded, &ds);
		/* output the encoded bytes to the output file */
		fwrite(decoded, sizeof(char), cnt, outputFile);
	}
	/*---------- START DECODING  ----------*/
	
	free(encoded);
	free(decoded);
}

int BinaryFileEncode(const char *filepath,int sockConn)
{
	FILE* inputFile;
	FILE* encodedFile;
	
	
	/* encode the input file */
	
	inputFile   = fopen(filepath, "r");
	encodedFile = fopen("encodedfile", "w");
	
	encode(inputFile, encodedFile);
	
	fclose(inputFile);
	fclose(encodedFile);

}
void BinaryFileDecode(const char *filepath)
{
	FILE* encodedFile;
	FILE* decodedFile;
	encodedFile = fopen("encodedfile", "r");
	decodedFile = fopen(filepath, "w");
	
	decode(encodedFile, decodedFile);
	
	fclose(encodedFile);
	fclose(decodedFile);

}


