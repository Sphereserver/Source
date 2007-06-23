using System;

namespace SpyUO
{
	public enum Register { Edi, Esi, Ebx, Edx, Ecx, Eax, Ebp, Esp }

	public struct AddressAndRegisters
	{
		public uint Address;
		public Register AddressRegister;
        public uint LengthAddress;
        public Register LengthRegister;

		public AddressAndRegisters( uint address, Register addressRegister, uint lenaddress, Register lengthRegister )
		{
			Address = address;
			AddressRegister = addressRegister;
            LengthAddress = lenaddress;
			LengthRegister = lengthRegister;
		}
	}
}