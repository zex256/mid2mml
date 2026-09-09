#ifndef ENDIAN
#define ENDIAN
using namespace std;
// reverse endian
inline void reverse_endian(WORD &Val) {
	swap((reinterpret_cast<char*>(&Val))[0], (reinterpret_cast<char*>(&Val))[1]);
}
inline void reverse_endian(short &Val) {
	swap((reinterpret_cast<char*>(&Val))[0], (reinterpret_cast<char*>(&Val))[1]);
}
inline void reverse_endian(DWORD &Val) {
	swap((reinterpret_cast<char*>(&Val))[0], (reinterpret_cast<char*>(&Val))[3]);
	swap((reinterpret_cast<char*>(&Val))[1], (reinterpret_cast<char*>(&Val))[2]);
}
inline void reverse_endian(int &Val) {
	swap((reinterpret_cast<char*>(&Val))[0], (reinterpret_cast<char*>(&Val))[3]);
	swap((reinterpret_cast<char*>(&Val))[1], (reinterpret_cast<char*>(&Val))[2]);
}
inline void reverse_endian(long &Val) {
	swap((reinterpret_cast<char*>(&Val))[0], (reinterpret_cast<char*>(&Val))[3]);
	swap((reinterpret_cast<char*>(&Val))[1], (reinterpret_cast<char*>(&Val))[2]);
}
// convert endian
inline WORD convert_endian(const WORD &Val) {
	WORD Ret(Val);
	swap((reinterpret_cast<char*>(&Ret))[0], (reinterpret_cast<char*>(&Ret))[1]);
	return Ret;
}
inline short convert_endian(const short &Val) {
	short Ret(Val);
	swap((reinterpret_cast<char*>(&Ret))[0], (reinterpret_cast<char*>(&Ret))[1]);
	return Ret;
}
inline DWORD convert_endian(const DWORD &Val) {
	DWORD Ret(Val);
	swap((reinterpret_cast<char*>(&Ret))[0], (reinterpret_cast<char*>(&Ret))[3]);
	swap((reinterpret_cast<char*>(&Ret))[1], (reinterpret_cast<char*>(&Ret))[2]);
	return Ret;
}
inline int convert_endian(const int &Val) {
	int Ret(Val);
	swap((reinterpret_cast<char*>(&Ret))[0], (reinterpret_cast<char*>(&Ret))[3]);
	swap((reinterpret_cast<char*>(&Ret))[1], (reinterpret_cast<char*>(&Ret))[2]);
	return Ret;
}
inline long convert_endian(const long &Val) {
	long Ret(Val);
	swap((reinterpret_cast<char*>(&Ret))[0], (reinterpret_cast<char*>(&Ret))[3]);
	swap((reinterpret_cast<char*>(&Ret))[1], (reinterpret_cast<char*>(&Ret))[2]);
	return Ret;
}
#endif	// ENDIAN
