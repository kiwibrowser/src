#if API < 11
extern "C" HRESULT STDMETHODCALLTYPE D3D10CreateBlob(
	SIZE_T num_bytes,
	LPD3D10BLOB *out_buffer
);

HRESULT STDMETHODCALLTYPE D3D10CreateBlob(
	SIZE_T num_bytes,
	LPD3D10BLOB *out_buffer
)
{
	void* data = malloc(num_bytes);
	if(!data)
		return E_OUTOFMEMORY;
	*out_buffer = new GalliumD3DBlob(data, num_bytes);
	return S_OK;
}

LPCSTR STDMETHODCALLTYPE D3D10GetPixelShaderProfile(
	ID3D10Device *device
)
{
	return "ps_4_0";
}

LPCSTR STDMETHODCALLTYPE D3D10GetVertexShaderProfile(
	ID3D10Device *device
)
{
	return "vs_4_0";
}

LPCSTR STDMETHODCALLTYPE D3D10GetGeometryShaderProfile(
	ID3D10Device *device
)
{
	return "gs_4_0";
}

static HRESULT dxbc_assemble_as_blob(struct dxbc_chunk_header** chunks, unsigned num_chunks, ID3D10Blob** blob)
{
	std::pair<void*, size_t> p = dxbc_assemble(chunks, num_chunks);
	if(!p.first)
		return E_OUTOFMEMORY;
	*blob = new GalliumD3DBlob(p.first, p.second);
	return S_OK;
}

HRESULT D3D10GetInputSignatureBlob(
	const void *shader_bytecode,
	SIZE_T bytecode_length,
	ID3D10Blob **signature_blob
)
{
	dxbc_chunk_signature* sig = dxbc_find_signature(shader_bytecode, bytecode_length, DXBC_FIND_INPUT_SIGNATURE);
	if(!sig)
		return E_FAIL;

	return dxbc_assemble_as_blob((dxbc_chunk_header**)&sig, 1, signature_blob);
}

HRESULT D3D10GetOutputSignatureBlob(
	const void *shader_bytecode,
	SIZE_T bytecode_length,
	ID3D10Blob **signature_blob
)
{
	dxbc_chunk_signature* sig = dxbc_find_signature(shader_bytecode, bytecode_length, DXBC_FIND_OUTPUT_SIGNATURE);
	if(!sig)
		return E_FAIL;

	return dxbc_assemble_as_blob((dxbc_chunk_header**)&sig, 1, signature_blob);
}

HRESULT D3D10GetInputAndOutputSignatureBlob(
	const void *shader_bytecode,
	SIZE_T bytecode_length,
	ID3D10Blob **signature_blob
)
{
	dxbc_chunk_signature* sigs[2];
	sigs[0] = dxbc_find_signature(shader_bytecode, bytecode_length, DXBC_FIND_INPUT_SIGNATURE);
	if(!sigs[0])
		return E_FAIL;
	sigs[1] = dxbc_find_signature(shader_bytecode, bytecode_length, DXBC_FIND_OUTPUT_SIGNATURE);
	if(!sigs[1])
		return E_FAIL;

	return dxbc_assemble_as_blob((dxbc_chunk_header**)&sigs, 2, signature_blob);
}

#endif
