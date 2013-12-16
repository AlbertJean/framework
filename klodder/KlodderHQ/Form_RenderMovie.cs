﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace KlodderHQ
{
    public partial class Form_RenderMovie : Form
    {
        public Form_RenderMovie()
        {
            InitializeComponent();

            int r1 = Settings.Get("back1_r", 250);
            int g1 = Settings.Get("back1_g", 250);
            int b1 = Settings.Get("back1_b", 250);
            int r2 = Settings.Get("back2_r", 240);
            int g2 = Settings.Get("back2_g", 240);
            int b2 = Settings.Get("back2_b", 240);
            userControl_RenderOptions1.BackColor1 = Color.FromArgb(r1, g1, b1);
            userControl_RenderOptions1.BackColor2 = Color.FromArgb(r2, g2, b2);
            userControl_RenderOptions1.RenderScale = Settings.Get("scale", 1);
            userControl_RenderOptions1.Preview = Settings.Get("preview", true);
            userControl_MovieOptions1.FrameRate = Settings.Get("fps", 25);
            userControl_MovieOptions1.PauseFrames = Settings.Get("trail", 25);

            userControl_RenderOptions1.Source = Form1.DefaultForm.FileName;
            userControl_MovieOptions1.Destination = Path.ChangeExtension(Form1.DefaultForm.FileName, "mov");
        }

        private void button1_Click(object sender, EventArgs args)
        {
            try
            {
                int r1 = userControl_RenderOptions1.BackColor1.R;
                int g1 = userControl_RenderOptions1.BackColor1.G;
                int b1 = userControl_RenderOptions1.BackColor1.B;
                int r2 = userControl_RenderOptions1.BackColor2.R;
                int g2 = userControl_RenderOptions1.BackColor2.G;
                int b2 = userControl_RenderOptions1.BackColor2.B;

                Settings.Set("back1_r", r1);
                Settings.Set("back1_g", g1);
                Settings.Set("back1_b", b1);
                Settings.Set("back2_r", r2);
                Settings.Set("back2_g", g2);
                Settings.Set("back2_b", b2);
                Settings.Set("scale", userControl_RenderOptions1.RenderScale);
                Settings.Set("preview", userControl_RenderOptions1.Preview);
                Settings.Set("fps", userControl_MovieOptions1.FrameRate);
                Settings.Set("trail", userControl_MovieOptions1.PauseFrames);

                StaticMethods.ExecuteKlodder(
                    Form1.Debug,
                    userControl_RenderOptions1.Source,
                    "-m",
                    "movie",
                    "-s",
                    userControl_RenderOptions1.RenderScale.ToString(),
                    "-b1",
                    userControl_RenderOptions1.BackColor1.R.ToString(),
                    userControl_RenderOptions1.BackColor1.G.ToString(),
                    userControl_RenderOptions1.BackColor1.B.ToString(),
                    "-b2",
                    userControl_RenderOptions1.BackColor2.R.ToString(),
                    userControl_RenderOptions1.BackColor2.G.ToString(),
                    userControl_RenderOptions1.BackColor2.B.ToString(),
                    "-o",
                    userControl_MovieOptions1.Destination,
                    "-p",
                    userControl_RenderOptions1.Preview ? "1" : "0");

                Close();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.Message);
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            Close();
        }
    }
}
