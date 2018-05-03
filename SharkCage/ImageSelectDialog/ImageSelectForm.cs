using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SharkCage
{
    public partial class ImageSelectForm : Form
    {
        public ImageSelectForm(string configPath)
        {
            InitializeComponent();

            file = new System.IO.StreamWriter(configPath);
            pictureBox1.Click += new EventHandler(PictureOneClick);
            pictureBox2.Click += new EventHandler(PictureTwoClick);
            pictureBox3.Click += new EventHandler(PictureThreeClick);
        }
    }
}
