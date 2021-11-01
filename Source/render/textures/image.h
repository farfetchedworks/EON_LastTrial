#pragma once

class CTexture;

template <typename T> class TImage
{
public:
	unsigned int width;
	unsigned int height;
	unsigned int num_channels;	//bits per pixel
	unsigned int num_faces;		//cubemaps faces
	T* data;					//bytes with the pixel information

	TImage() { width = height = 0; data = nullptr; num_channels = 4; }
	TImage(int w, int h, int num_channels = 4) { data = nullptr; resize(w, h, num_channels); }
	~TImage() { if (data) delete[] data; data = nullptr; }

	void resize(int w, int h, int num_channels = 4, int num_faces = 1) { 
		if (data) delete[] data; 
		width = w; height = h; this->num_channels = num_channels; this->num_faces = num_faces;
		data = new T[w * h * num_channels * num_faces];
		std::memset(data, 0, w * h * sizeof(T) * num_channels * num_faces);
	}

	void clear() { 
		if (data) delete[] data; data = nullptr; 
		width = height = 0; 
	}
};

class CFloatImage : public TImage<float>
{
public:
	VEC4 getPixel(int x, int y) const {
		assert(x >= 0 && x < (int)width&& y >= 0 && y < (int)height && "reading of memory");
		int pos = y * width * num_channels + x * num_channels;
		return VEC4(data[pos], data[pos + 1], data[pos + 2], num_channels == 3 ? 1 : data[pos + 3]);
	};
	void setPixel(int x, int y, VEC4 v) {
		assert(x >= 0 && x < (int)width&& y >= 0 && y < (int)height && "writing of memory");
		int pos = y * width * num_channels + x * num_channels;
		data[pos] = v.x; data[pos + 1] = v.y; data[pos + 2] = v.z;
		if (num_channels == 4)
			data[pos + 3] = v.w;
	};

	void fromTexture(CTexture* texture) {
		assert(texture);
		ID3D11Texture2D* tex2D = (ID3D11Texture2D*)texture->getDXResource();
		D3D11_TEXTURE2D_DESC desc;
		tex2D->GetDesc(&desc);
		resize(desc.Width, desc.Height);
		texture->copyToCPU(data, width, height);
	};
};

class CFloatCubemap : public TImage<float>
{
public:
	VEC4 getPixel(int x, int y, int cube_face) const {
		assert(x >= 0 && x < (int)width&& y >= 0 && y < (int)height && "reading of memory");
		int pos = cube_face * width * height * num_channels + y * width * num_channels + x * num_channels;
		return VEC4(data[pos], data[pos + 1], data[pos + 2], num_channels == 3 ? 1 : data[pos + 3]);
	};
	void setPixel(int x, int y, VEC4 v, int cube_face) {
		assert(x >= 0 && x < (int)width&& y >= 0 && y < (int)height && "writing of memory");
		int pos = cube_face * width * height * num_channels + y * width * num_channels + x * num_channels;
		data[pos] = v.x; data[pos + 1] = v.y; data[pos + 2] = v.z;
		if (num_channels == 4)
			data[pos + 3] = v.w;
	};

	void fromTexture(CTexture* texture) {
		assert(texture);
		ID3D11Texture2D* tex2D = (ID3D11Texture2D*)texture->getDXResource();
		D3D11_TEXTURE2D_DESC desc;
		tex2D->GetDesc(&desc);
		resize(desc.Width, desc.Height, 4, 6);
		texture->copyToCPU(data, width, height, 6);
	};
};