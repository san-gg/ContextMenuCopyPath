
int CopyToClipboard(const char*);

int main(int argc, char ** argv)
{
	if (argc != 2) return 1;
	return CopyToClipboard(argv[1]);
}