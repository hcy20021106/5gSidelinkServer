/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "conversions.h"
#include "secu_defs.h"

static
void eia1_encrypt(char* test_nameP, uint8_t direction, uint32_t count,
                  uint8_t bearer, uint8_t *key, uint32_t key_length, uint8_t *message,
                  uint32_t length, uint8_t *expected, uint32_t length_expected)
{
  nas_stream_cipher_t nas_cipher;
  uint8_t result[4];

  nas_cipher.direction  = direction;
  nas_cipher.count      = count;
  nas_cipher.key        = key;
  nas_cipher.key_length = key_length;
  nas_cipher.bearer     = bearer;
  nas_cipher.blength    = length;
  nas_cipher.message    = message;

  if (nas_stream_encrypt_eia1(&nas_cipher, result) != 0) {
    fail("%s Fail: nas_stream_encrypt_eia1\n", test_nameP);
  }

  if (compare_buffer(result, 4, expected, length_expected) != 0) {
    fail("%s Fail: eia1_encrypt\n",test_nameP);
  } else {
    success("%s Success: eia1_encrypt\n",test_nameP);
  }
}

void doit (void)
{
  /* Test suite from Specification 3GPP TS 33.401 V12.10.0 #C.4 128-EIA1 */

  /* Test set 1 #C.4.1 */
  eia1_encrypt(
    "Test set 1 #C.4.1",
    0, 0x38a6f056, 0x1f, HL("2bd6459f82c5b300952c49104881ff48"),
    H("33323462633938613734790000000000"),
    88,
    HL("731f1165")
  );
  /* Test set 2 #C.4.2 */
  eia1_encrypt(
    "Test set 2 #C.4.2",
    1, 0x36af6144, 0x18, HL("7e5e94431e11d73828d739cc6ced4573"),
    H("b3d3c9170a4e1632f60f861013d22d84b726b6a278d802d1eeaf1321ba5929dc "),
    254,
    HL("e3259f6f")
  );
  /* Test set 3 #C.4.3 */
  eia1_encrypt(
    "Test set 3 #C.4.3",
    0, 0xc7590ea9, 0x17, HL("d3419be821087acd02123a9248033359"),
    H("bbb057038809496bcff86d6fbc8ce5b135a06b166054f2d565be8ace75dc851e0bcdd8f07141c495872fb5d8c0c66a8b6da556663e4e461205d84580bee5bc7e"),
    511,
    HL("9a16c77d")
  );
  /* Test set 4 #C.4.4 */
  eia1_encrypt(
    "Test set 4 #C.4.4",
    1, 0x36af6144, 0x0f, HL("83fd23a244a74cf358da3019f1722635"),
    H("35c68716633c66fb750c266865d53c11ea05b1e9fa49c8398d48e1efa5909d3947902837f5ae96d5a05bc8d61ca8dbef1b13a4b4abfe4fb1006045b674bb5472 9304c382be53a5af05556176f6eaa2ef1d05e4b083181ee674cda5a485f74d7a"),
    768,
    HL("bba74492")
  );
  /* Test set 5 #C.4.5 */
  eia1_encrypt(
    "Test set 5 #C.4.5",
    0, 0x36af6144, 0x18, HL("6832a65cff4473621ebdd4ba26a921fe"),
    H("d3c53839626820717765667620323837636240981ba6824c1bfb1ab485472029b71d808ce33e2cc3c0b5fc1f3de8a6dc"),
    383,
    HL("4145e4b0")
  );
  /* Test set 6 #C.4.6 */
  eia1_encrypt(
    "Test set 6 #C.4.6",
    1, 0x7827fab2, 0x05, HL("5d0a80d8134ae19677824b671e838af4 "),
    H("70dedf2dc42c5cbd3a96f8a0b11418b3608d5733604a2cd36aabc70ce3193bb5153be2d3c06dfdb2d16e9c357158be6a41d6b861e491db3fbfeb518efcf048d7d58953730ff30c9ec470ffcd663dc34201c36addc0111c35b38afee7cfdb582e3731f8b4baa8d1a89c06e81199a9716227be344efcb436ddd0f096c064c3b5e2c399993fc77394f9e09720a811850ef23b2ee05d9e6173609d86e1c0c18ea51a012a00bb413b9cb8188a703cd6bae31cc67b34b1b00019e6a2b2a690f02671fe7c9ef8dec0094e533763478d58d2c5f5b827a0148c5948a96931acf84f465a64e62ce74007e991e37ea823fa0fb21923b79905b733b631e6c7d6860a3831ac351a9c730c52ff72d9d308eedbab21fde143a0ea17e23edc1f74cbb3638a2033aaa15464eaa733385dbbeb6fd73509b857e6a419dca1d8907af977fbac4dfa35ec"),
    2558,
    HL("0fa2b1ee")
  );
  /* Test set 7 #C.4.7 */
  eia1_encrypt(
    "Test set 7 #C.4.7",
    1, 0x296f393c, 0x0b, HL("b3120ffdb2cf6af4e73eaf2ef4ebec69"),
    H("00000000000000000101010101010101e0958045f3a0bba4e3968346f0a3b8a7c02a018ae640765226b987c913e6cbf083570016cf83efbc61c082513e21561a427c009d28c298eface78ed6d56c2d4505ad032e9c04dc60e73a81696da665c6c48603a57b45ab33221585e68ee3169187fb0239528632dd656c807ea3248b7b46d002b2b5c7458eb85b9ce95879e0340859055e3b0abbc3eace8719caa80265c97205d5dc4bcc902fe1839629ed71328a0f0449f588557e6898860e042aecd84b2404c212c9222da5bf8a89ef6797870cf50771a60f66a2ee62853657addf04cdde07fa414e11f12b4d81b9b4e8ac538ea30666688d881f6c348421992f31b94f8806ed8fccff4c9123b89642527ad613b109bf75167485f1268bf884b4cd23d29a0934925703d634098f7767f1be7491e708a8bb949a3873708aef4a36239e50cc08235cd5ed6bbe578668a17b58c1171d0b90e813a9e4f58a89d719b11042d6360b1b0f52deb730a58d58faf46315954b0a872691475977dc88c0d733feff54600a0cc1d0300aaaeb94572c6e95b01ae90de04f1dce47f87e8fa7bebf77e1dbc20d6ba85cb9143d518b285dfa04b698bf0cf7819f20fa7a288eb0703d995c59940c7c66de57a9b70f82379b70e2031e450fcfd2181326fcd28d8823baaa80df6e0f443559647539fd8907c0ffd9d79c130ed81c9afd9b7e848c9fed38443d5d380e53fbdb8ac8c3d3f06876054f122461107de92fea09c6f6923a188d53afe54a10f60e6e9d5a03d996b5fbc820f8a637116a27ad04b444a0932dd60fbd12671c11e1c0ec73e789879faa3d42c64d20cd1252742a3768c25a901585888ecee1e612d9936b403b0775949a66cdfd99a29b1345baa8d9d5400c91024b0a607363b013ce5de9ae869d3b8d95b0570b3c2d391422d32450cbcfae96652286e96dec1214a9346527980a8192eac1c39a3aaf6f15351da6be764df89772ec0407d06e4415befae7c92580df9bf507497c8f2995160d4e218daacb02944abf83340ce8be1686a960faf90e2d90c55cc6475babc3171a80a363174954955d7101dab16ae8179167e21444b443a9eaaa7c91de36d118c39d389f8dd4469a846c9a262bf7fa18487a79e8de11699e0b8fdf557cb48719d453ba713056109b93a218c89675ac195fb4fb06639b3797144955b3c9327d1aec003d42ecd0ea98abf19ffb4af3561a67e77c35bf15c59c2412da881db02b1bfbcebfac5152bc99bc3f1d15f771001b7029fedb028f8b852bc4407eb83f891c9ca733254fdd1e9edb56919ce9fea21c174072521c18319a54b5d4efbebddf1d8b69b1cbf25f489fcc981372547cf41d008ef0bca1926f934b735e090b3b251eb33a36f82ed9b29cf4cb944188fa0e1e38dd778f7d1c9d987b28d132dfb9731fa4f4b416935be49de30516af3578581f2f13f561c0663361941eab249a4bc123f8d15cd711a956a1bf20fe6eb78aea2373361da0426c79a530c3bb1de0c99722ef1fde39ac2b00a0a8ee7c800a08bc2264f89f4effe627ac2f0531fb554f6d21d74c590a70adfaa390bdfbb3d68e46215cab187d2368d5a71f5ebec081cd3b20c082dbe4cd2faca28773795d6b0c10204b659a939ef29bbe1088243624429927a7eb576dd3a00ea5e01af5d47583b2272c0c161a806521a16ff9b0a722c0cf26b025d5836e2258a4f7d4773ac801e4263bc294f43def7fa8703f3a4197463525887652b0b2a4a2a7cf87f00914871e25039113c7e1618da34064b57a43c463249fb8d05e0f26f4a6d84972e7a9054824145f91295cdbe39a6f920facc659712b46a54ba295bbe6a90154e91b33985a2bcd420ad5c67ec9ad8eb7ac6864db272a516bc94c2839b0a8169a6bf58e1a0c2ada8c883b7bf497a49171268ed15ddd2969384e7ff4bf4aab2ec9ecc6529cf629e2df0f08a77a65afa12aa9b505df8b287ef6cc91493d1caa39076e28ef1ea028f5118de61ae02bb6aefc3343a050292f199f401857b2bead5e6ee2a1f191022f9278016f047791a9d18da7d2a6d27f2e0e51c2f6ea30e8ac49a0604f4c13542e85b68381b9fdcfa0ce4b2d341354852d360245c536b612af71f3e77c9095ae2dbde504b265733dabfe10a20fc7d6d32c21ccc72b8b3444ae663d65922d17f82caa2b865cd88913d291a65899026ea1328439723c198c36b0c3c8d085bfaf8a320fde334b4a4919b44c2b95f6e8ecf73393f7f0d2a40e60b1d406526b022ddc331810b1a5f7c347bd53ed1f105d6a0d30aba477e178889ab2ec55d558deab2630204336962b4db5b663b6902b89e85b31bc6af50fc50accb3fb9b57b663297031378db47896d7fbaf6c600add2c67f936db037986db856eb49cf2db3f7da6d23650e438f1884041b013119e4c2ae5af37cccdfb68660738b58b3c59d1c0248437472aba1f35ca1fb90cd714aa9f635534f49e7c5bba81c2b6b36fdee21ca27e347f793d2ce944edb23c8c9b914be10335e350feb5070394b7a4a15c0ca120283568b7bfc254fe838b137a2147ce7c113a3a4d65499d9e86b87dbcc7f03bbd3a3ab1aa243ece5ba9bcf25f82836cfe473b2d83e7a7201cd0b96a72451e863f6c3ba664a6d073d1f7b5ed990865d978bd3815d06094fc9a2aba5221c22d5ab996389e3721e3af5f05beddc2875e0dfaeb39021ee27a41187cbb45ef40c3e73bc03989f9a30d12c54ba7d2141da8a875493e65776ef35f97debc2286cc4af9b4623eee902f840c52f1b8ad658939aef71f3f72b9ec1de21588bd35484ea44436343ff95ead6ab1d8afb1b2a303df1b71e53c4aea6b2e3e9372be0d1bc99798b0ce3cc10d2a596d565dba82f88ce4cff3b33d5d24e9c0831124bf1ad54b792532983dd6c3a8b7d0"),
    16448,
    HL("abf3e651")
  );
}
