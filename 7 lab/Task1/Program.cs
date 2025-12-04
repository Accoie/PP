using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Task1
{
    public class Program
    {
        public static async Task Main()
        {
            Console.Write( "Укажите путь к файлу: " );
            string? path = Console.ReadLine();

            Console.Write( "Введите набор символов для удаления: " );
            string? forbidden = Console.ReadLine();

            if ( string.IsNullOrWhiteSpace( path ) || !File.Exists( path ) )
            {
                Console.WriteLine( "Некорректный путь к файлу." );

                return;
            }

            try
            {
                string text = await File.ReadAllTextAsync( path );
                StringBuilder builder = new StringBuilder();

                foreach ( char ch in text )
                {
                    if ( forbidden is null || !forbidden.Contains( ch ) )
                    {
                        builder.Append( ch );
                    }
                }

                await File.WriteAllTextAsync( path, builder.ToString() );

                Console.WriteLine( "Обработка завершена успешно." );
            }
            catch ( IOException )
            {
                Console.WriteLine( "Ошибка при работе с файлом." );
            }
            catch ( Exception e )
            {
                Console.WriteLine( $"Неожиданная ошибка: {e.Message}" );
            }
        }
    }
}
