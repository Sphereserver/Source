using System;
using System.Text;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using SpyUO.Packets;

namespace SpyUO
{
	public class PacketDetails : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label lType;
		private System.Windows.Forms.TextBox tbType;
		private System.Windows.Forms.Label lTime;
		private System.Windows.Forms.TextBox tbTime;
		private System.Windows.Forms.Label lRelTime;
		private System.Windows.Forms.TextBox tbRelTime;
		private System.Windows.Forms.Label lDifTime;
		private System.Windows.Forms.TextBox tbDifTime;
		private System.Windows.Forms.TextBox tbMessage;
		private System.Windows.Forms.TextBox tbPacket;
		private System.Windows.Forms.Label lLength;
		private System.Windows.Forms.TextBox tbLength;

		private System.ComponentModel.Container components = null;

		public PacketDetails( PacketListViewItem item )
		{
			InitializeComponent();

			tbType.Text = item.GetPacketType();
			tbTime.Text = item.GetTime();
			tbRelTime.Text = item.GetRelTime();
			tbDifTime.Text = item.GetDifTime();
			tbLength.Text = item.GetLength();
			tbMessage.Text = GetMessage( item );
			tbPacket.Text = GetPacket( item );
		}

		public static string GetMessage( PacketListViewItem item )
		{
			PacketProp[] props = item.TimePacket.Packet.GetPacketProperties();
			if ( props == null )
				return "Unknown message";
			else if ( props.Length == 0 )
				return "Empty message";

			StringBuilder sb = new StringBuilder();
			int i = 0;
			while ( true )
			{
				PacketProp prop = props[i];
				sb.Append( "- " + prop.PropInfo.Name + ": \"" + prop.GetStringValue() + "\"" );

				if ( ++i < props.Length )
					sb.Append( "\r\n" );
				else
					break;
			}
			return sb.ToString();
		}

		public static string GetPacket( PacketListViewItem item )
		{
			Packet packet = item.TimePacket.Packet;
			byte[] data = packet.Data;

			StringBuilder sb = new StringBuilder();

			for ( int l = 0; l < data.Length; l += 0x10 )
			{
				sb.AppendFormat( "{0:X4}:", l );

				for ( int i = l; i < l + 0x10; i++ )
					sb.AppendFormat( " {0}", i < data.Length ? data[i].ToString( "X2" ) : "--" );

				sb.Append( "\t" );

				for ( int i = l; i < l + 0x10; i++ )
				{
					if ( i >= data.Length )
						break;

					byte b = data[i];
					char c;
					if ( b >= 0x20 && b < 0x80 )
						c = (char)b;
					else
						c = '.';

					sb.Append( c );
				}

				sb.Append( "\r\n" );
			}

			return sb.ToString();
		}

		protected override void Dispose( bool disposing )
		{
			if ( disposing )
			{
				if ( components != null )
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Metodo necessario per il supporto della finestra di progettazione. Non modificare
		/// il contenuto del metodo con l'editor di codice.
		/// </summary>
		private void InitializeComponent()
		{
			this.lType = new System.Windows.Forms.Label();
			this.tbType = new System.Windows.Forms.TextBox();
			this.lTime = new System.Windows.Forms.Label();
			this.tbTime = new System.Windows.Forms.TextBox();
			this.lRelTime = new System.Windows.Forms.Label();
			this.tbRelTime = new System.Windows.Forms.TextBox();
			this.lDifTime = new System.Windows.Forms.Label();
			this.tbDifTime = new System.Windows.Forms.TextBox();
			this.tbMessage = new System.Windows.Forms.TextBox();
			this.tbPacket = new System.Windows.Forms.TextBox();
			this.lLength = new System.Windows.Forms.Label();
			this.tbLength = new System.Windows.Forms.TextBox();
			this.SuspendLayout();
			// 
			// lType
			// 
			this.lType.Location = new System.Drawing.Point(8, 10);
			this.lType.Name = "lType";
			this.lType.Size = new System.Drawing.Size(32, 16);
			this.lType.TabIndex = 0;
			this.lType.Text = "Type:";
			// 
			// tbType
			// 
			this.tbType.Font = new System.Drawing.Font("Arial Unicode MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.tbType.Location = new System.Drawing.Point(40, 8);
			this.tbType.Name = "tbType";
			this.tbType.ReadOnly = true;
			this.tbType.Size = new System.Drawing.Size(504, 22);
			this.tbType.TabIndex = 1;
			this.tbType.Text = "";
			// 
			// lTime
			// 
			this.lTime.Location = new System.Drawing.Point(16, 42);
			this.lTime.Name = "lTime";
			this.lTime.Size = new System.Drawing.Size(40, 16);
			this.lTime.TabIndex = 2;
			this.lTime.Text = "Time:";
			// 
			// tbTime
			// 
			this.tbTime.Font = new System.Drawing.Font("Arial Unicode MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.tbTime.Location = new System.Drawing.Point(48, 40);
			this.tbTime.Name = "tbTime";
			this.tbTime.ReadOnly = true;
			this.tbTime.Size = new System.Drawing.Size(80, 22);
			this.tbTime.TabIndex = 3;
			this.tbTime.Text = "";
			// 
			// lRelTime
			// 
			this.lRelTime.Location = new System.Drawing.Point(136, 42);
			this.lRelTime.Name = "lRelTime";
			this.lRelTime.Size = new System.Drawing.Size(56, 16);
			this.lRelTime.TabIndex = 4;
			this.lRelTime.Text = "Rel time:";
			// 
			// tbRelTime
			// 
			this.tbRelTime.Font = new System.Drawing.Font("Arial Unicode MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.tbRelTime.Location = new System.Drawing.Point(184, 40);
			this.tbRelTime.Name = "tbRelTime";
			this.tbRelTime.ReadOnly = true;
			this.tbRelTime.Size = new System.Drawing.Size(80, 22);
			this.tbRelTime.TabIndex = 5;
			this.tbRelTime.Text = "";
			// 
			// lDifTime
			// 
			this.lDifTime.Location = new System.Drawing.Point(272, 42);
			this.lDifTime.Name = "lDifTime";
			this.lDifTime.Size = new System.Drawing.Size(48, 16);
			this.lDifTime.TabIndex = 6;
			this.lDifTime.Text = "Dif time:";
			// 
			// tbDifTime
			// 
			this.tbDifTime.Font = new System.Drawing.Font("Arial Unicode MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.tbDifTime.Location = new System.Drawing.Point(320, 40);
			this.tbDifTime.Name = "tbDifTime";
			this.tbDifTime.ReadOnly = true;
			this.tbDifTime.Size = new System.Drawing.Size(80, 22);
			this.tbDifTime.TabIndex = 7;
			this.tbDifTime.Text = "";
			// 
			// tbMessage
			// 
			this.tbMessage.Font = new System.Drawing.Font("Arial Unicode MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.tbMessage.Location = new System.Drawing.Point(8, 72);
			this.tbMessage.Multiline = true;
			this.tbMessage.Name = "tbMessage";
			this.tbMessage.ReadOnly = true;
			this.tbMessage.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.tbMessage.Size = new System.Drawing.Size(536, 152);
			this.tbMessage.TabIndex = 8;
			this.tbMessage.Text = "";
			// 
			// tbPacket
			// 
			this.tbPacket.Font = new System.Drawing.Font("Lucida Console", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.tbPacket.Location = new System.Drawing.Point(8, 232);
			this.tbPacket.Multiline = true;
			this.tbPacket.Name = "tbPacket";
			this.tbPacket.ReadOnly = true;
			this.tbPacket.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.tbPacket.Size = new System.Drawing.Size(536, 152);
			this.tbPacket.TabIndex = 9;
			this.tbPacket.Text = "";
			// 
			// lLength
			// 
			this.lLength.Location = new System.Drawing.Point(408, 42);
			this.lLength.Name = "lLength";
			this.lLength.Size = new System.Drawing.Size(48, 16);
			this.lLength.TabIndex = 10;
			this.lLength.Text = "Length:";
			// 
			// tbLength
			// 
			this.tbLength.Font = new System.Drawing.Font("Arial Unicode MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.tbLength.Location = new System.Drawing.Point(456, 40);
			this.tbLength.Name = "tbLength";
			this.tbLength.ReadOnly = true;
			this.tbLength.Size = new System.Drawing.Size(80, 22);
			this.tbLength.TabIndex = 11;
			this.tbLength.Text = "";
			// 
			// PacketDetails
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(554, 392);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.tbLength,
																		  this.lLength,
																		  this.tbPacket,
																		  this.tbMessage,
																		  this.tbDifTime,
																		  this.lDifTime,
																		  this.tbRelTime,
																		  this.lRelTime,
																		  this.tbTime,
																		  this.lTime,
																		  this.tbType,
																		  this.lType});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "PacketDetails";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Packet details";
			this.ResumeLayout(false);

		}
		#endregion
	}
}
