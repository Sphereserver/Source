namespace SpyUO
{
	public interface ICounterDisplay
	{
		void DisplayCounter( int sentPackets, int sentPacketsSize, int recvPackets, int recvPacketsSize );
	}
}